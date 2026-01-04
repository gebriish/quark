#include "quark_core.h"

#include "../os/os_inc.h"

global Quark_Ctx *_quark_ctx;

#define MAX_UNDO_DEPTH 128

internal Quark_Ctx 
quark_new()
{
	Quark_Ctx result = {0};

	result.arena       = arena_new(GB(4), os_reserve, os_commit, os_decommit, os_release);
	result.frame_arena = arena_new(GB(4), os_reserve, os_commit, os_decommit, os_release);
	result.buffer_arena= arena_new(GB(4), os_reserve, os_commit, os_decommit, os_release);

	if (!result.arena) {
		log_error("Failed to create main arena"); Trap();
	}

	if (!result.frame_arena) {
		log_error("Failed to create frame arena"); Trap();
	}

	if (!result.buffer_arena) {
		log_error("Failed to create buffer arena"); Trap();
	}

	result.undo_queue = cmd_queue_make(result.arena, MAX_UNDO_DEPTH);
	result.redo_queue = cmd_queue_make(result.arena, MAX_UNDO_DEPTH);

	return result;
}

internal void 
quark_delete()
{
	Assert(_quark_ctx);

	arena_delete(_quark_ctx->arena);
	arena_delete(_quark_ctx->frame_arena);
	arena_delete(_quark_ctx->buffer_arena);

	_quark_ctx = NULL;
}


internal Quark_Ctx *
quark_set_context(Quark_Ctx *ctx)
{
	Quark_Ctx *prev = _quark_ctx;
	_quark_ctx = ctx;
	return prev;
}

internal Quark_Error
quark_exec_cmd(Quark_Cmd cmd)
{
	Assert(_quark_ctx);

	// TODO: exec cmd
	
	if (!cmd_queue_push_back(&_quark_ctx->undo_queue, cmd)) {
		Quark_Cmd old_cmd = {0};
		cmd_queue_pop_front(&_quark_ctx->undo_queue, &old_cmd);
		cmd_queue_push_back(&_quark_ctx->undo_queue, cmd);
	}

	cmd_queue_clear(&_quark_ctx->redo_queue);

	return Quark_Err_None;
}

internal Quark_Error
quark_undo()
{
	Assert(_quark_ctx);

	Quark_Cmd latest_cmd = {0};
	if (!cmd_queue_pop_back(&_quark_ctx->undo_queue, &latest_cmd)) {
		log_warn("redo queue empty");
		return Quark_Err_Underflow;
	}

	// TODO: exec reverse of latest_cmd

	if (!cmd_queue_push_back(&_quark_ctx->redo_queue, latest_cmd)) {
		Quark_Cmd old_cmd = {0};
		cmd_queue_pop_front(&_quark_ctx->redo_queue, &old_cmd);
		cmd_queue_push_back(&_quark_ctx->redo_queue, latest_cmd);
	}

	return Quark_Err_None;
}

internal Quark_Error
quark_redo()
{
	Assert(_quark_ctx);

	Quark_Cmd cmd = {0};
	if (!cmd_queue_pop_back(&_quark_ctx->redo_queue, &cmd)) {
		log_warn("redo queue empty");
		return Quark_Err_Underflow;
	}

	// TODO: exec forward of cmd

	if (!cmd_queue_push_back(&_quark_ctx->undo_queue, cmd)) {
		Quark_Cmd old_cmd = {0};
		cmd_queue_pop_front(&_quark_ctx->undo_queue, &old_cmd);
		cmd_queue_push_back(&_quark_ctx->undo_queue, cmd);
	}

	return Quark_Err_None;
}
