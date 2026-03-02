#include "editor.h"

///////////////////////////////////////////////
// ~geb: Command Handlers

internal void
_cmd_mode_change(Editor_Context *ctx, Mode_Change cmd)
{
	ctx->mode = cmd.to;
}

internal void
_cmd_buffer_open(Editor_Context *ctx, Buffer_New cmd)
{
	String8 src = os_data_from_path(cmd.name, ctx->frame_alloc);

	Q_Buffer *b = buffer_make(cmd.name,
	                          src,
	                          ctx->active_buffer,
	                          ctx->alloc);

	if (!b) return;

	ctx->active_buffer = b;

	if (!ctx->buffer_list)
		ctx->buffer_list = b;
}

internal void
_cmd_buffer_close(Editor_Context *ctx, Buffer_Close cmd)
{
	Q_Buffer *b = cmd.buffer;
	if (!b) return;

	if (ctx->active_buffer == b)
		ctx->active_buffer = b->next ? b->next : b->prev;

	if (ctx->buffer_list == b)
		ctx->buffer_list = b->next;

	buffer_delete(b);
}

internal void
_cmd_text_insert(Editor_Context *ctx, Text_Insert cmd)
{
	if (!ctx->active_buffer) return;

	ctx->active_buffer =
		buffer_insert(ctx->active_buffer, cmd.text);
}

internal void
_cmd_text_delete(Editor_Context *ctx, Text_Delete cmd)
{
	if (!ctx->active_buffer) return;

	Q_Buffer *buf = ctx->active_buffer;

	for (int i = 0; i < cmd.amount; ++i) {
		if (cmd.move) {
			if (buf->gap_pos <= 0) break;
			buffer_move_left(buf);
		}

		buffer_erase(buf);
	}
}

internal void
_cmd_cursor_move(Editor_Context *ctx, Cursor_Move cmd)
{
	if (!ctx->active_buffer) return;

	Q_Buffer *buf = ctx->active_buffer;

	if (cmd.dx < 0) {
		for (int i = 0; i < -cmd.dx; ++i)
			buffer_move_left(buf);
	}
	else if (cmd.dx > 0) {
		for (int i = 0; i < cmd.dx; ++i)
			buffer_move_right(buf);
	}

	if (cmd.dy < 0) {
		for (int i = 0; i < -cmd.dy; ++i)
			buffer_move_up(buf);
	}
	else if (cmd.dy > 0) {
		for (int i = 0; i < cmd.dy; ++i)
			buffer_move_down(buf);
	}
}

///////////////////////////////////////////////

internal Editor_Context
editor_context(Allocator alloc, Allocator frame_alloc)
{
	Editor_Context editor = {0};

	editor.mode = Mode_Normal;
	editor.alloc = alloc;
	editor.frame_alloc = frame_alloc;

	return editor;
}

internal void
editor_push_cmd(Editor_Context *ctx, Editor_Cmd cmd)
{
	switch (cmd.type) {
		case Cmd_Mode_Change:  _cmd_mode_change(ctx, cmd.mode); break;
		case Cmd_Buffer_Open:  _cmd_buffer_open(ctx, cmd.buffer_open); break;
		case Cmd_Buffer_Close: _cmd_buffer_close(ctx, cmd.buffer_close); break;
		case Cmd_Insert_Text:  _cmd_text_insert(ctx, cmd.text_insert); break;
		case Cmd_Delete_Text:  _cmd_text_delete(ctx, cmd.text_delete); break;
		case Cmd_Cursor_Move:  _cmd_cursor_move(ctx, cmd.cursor); break;
	}
}
