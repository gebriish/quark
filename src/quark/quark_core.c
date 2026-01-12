#include "quark_core.h"

#include "../os/os_inc.h"

global Quark_Ctx *_quark_ctx;

#define MAX_UNDO_DEPTH 128

////////////////////////////////////////////
// ~geb: Cmd Exec functions

typedef Quark_Error (*Quark_Cmd_Fn)(Quark_Cmd *);

internal Quark_Error
cmd_new_buffer(Quark_Cmd *cmd)
{
	String8 name = cmd->buffer_new.name;
	String8 data = cmd->buffer_new.data_buffer;

	QBuffer *buf = NULL;

	if (_quark_ctx->buffer_freelist.len > 0) {
		QBuffer *itr = _quark_ctx->buffer_freelist.first;

		while (itr) {
			if (itr->capacity >= data.len * 2) {
				qbuffer_list_remove(&_quark_ctx->buffer_freelist, itr);
				buf = itr;
				quark_buffer_clear(buf);
				quark_buffer_insert(buf, data, 0);
				break;
			}
			itr = itr->next;
		}
	}

	if (!buf) {
		buf = quark_buffer_new(_quark_ctx->buffer_arena, data);
	}

	buf->name = name;

	_quark_ctx->active_buffer = buf;
	qbuffer_list_push(&_quark_ctx->open_buffers, buf);

	return Quark_Err_None;
}

internal Quark_Error cmd_save_buffer(Quark_Cmd *cmd)
{
	QBuffer *buf = cmd->save_buffer.buf;

	String8 path_to_save = buf->name;

	if (path_to_save.len == 0) {
		return Quark_Err_File_Save_Failed;
	}

	usize len = quark_buffer_length(buf);

	String8 data = quark_buffer_slice(
		buf, 0, len, _quark_ctx->frame_arena
	);

	if (!os_write_data_to_file_path(path_to_save, data, _quark_ctx->frame_arena)) 
	{
		return Quark_Err_File_Save_Failed;
	}

	return Quark_Err_None;
}

internal Quark_Error
cmd_open_file (Quark_Cmd *cmd)
{
	String8 file_path = cmd->open_file.path;

	String8 file_data = os_data_from_file_path(
		_quark_ctx->frame_arena, file_path
	);

	Quark_Cmd new_buf_cmd = {0};
	new_buf_cmd.kind = Cmd_Kind_New_Buffer;
	new_buf_cmd.buffer_new.name = file_path;
	new_buf_cmd.buffer_new.data_buffer = file_data;

	quark_exec_cmd(new_buf_cmd);

	return Quark_Err_None;
}

internal Quark_Error
cmd_reset_editor(Quark_Cmd *cmd)
{
	_quark_ctx->state = State_Normal;

	_quark_ctx->open_buffers = (QBuffer_List){0};
	_quark_ctx->buffer_freelist = (QBuffer_List){0};
	_quark_ctx->active_buffer = NULL;

	arena_reset(_quark_ctx->buffer_arena);

	return Quark_Err_None;
}

internal Quark_Error
cmd_change_state(Quark_Cmd *cmd)
{
	Quark_State to_state = cmd->to_state;
	Quark_State from_state = _quark_ctx->state;

	return Quark_Err_None;
}

internal Quark_Cmd_Fn QUARK_CMD_EXEC[Cmd_Kind_Count] = {
	[Cmd_Kind_New_Buffer]    = cmd_new_buffer,
	[Cmd_Kind_Delete_Buffer] = NULL,
	[Cmd_Kind_Save_Buffer]   = cmd_save_buffer,
	[Cmd_Kind_Open_File]     = cmd_open_file,
	[Cmd_Kind_Reset]         = cmd_reset_editor,
};

////////////////////////////////////////////

internal Quark_Ctx 
quark_new()
{
	Quark_Ctx result = {0};
	
	result.state        = State_Normal;
	result.arena        = arena_new(GB(4), os_reserve, os_commit, os_decommit, os_release);
	result.frame_arena  = arena_new(GB(4), os_reserve, os_commit, os_decommit, os_release);
	result.buffer_arena = arena_new(GB(4), os_reserve, os_commit, os_decommit, os_release);

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
	if (cmd.kind >= Cmd_Kind_Count) {
		log_error("Command Kind %d is not valid", cmd.kind);
	}

	Quark_Cmd_Fn func = QUARK_CMD_EXEC[cmd.kind];

	if (func) {
		Quark_Error err = func(&cmd);
		if (err != Quark_Err_None) {
			log_error("Command Exec failed with Error(%d)", err);
			return err;
		}
	} else {
		log_error("Command kind %d's procedure is not implemented", cmd.kind);
	}

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

	switch(latest_cmd.kind) {
		case Cmd_Kind_Open_File: {
			// TODO: implement
		} break;
	}

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

	quark_exec_cmd(cmd);

	if (!cmd_queue_push_back(&_quark_ctx->undo_queue, cmd)) {
		Quark_Cmd old_cmd = {0};
		cmd_queue_pop_front(&_quark_ctx->undo_queue, &old_cmd);
		cmd_queue_push_back(&_quark_ctx->undo_queue, cmd);
	}

	return Quark_Err_None;
}


internal void
quark_update_state(Input_Data *data)
{
}
