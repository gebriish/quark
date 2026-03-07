#ifndef EDITOR_H
#define EDITOR_H

#include "base.h"
#include "buffer.h"

typedef u32 Editor_Mode;
enum {
	Mode_Normal = 0,
	Mode_Insert,
	Mode_Visual,
	Mode_CLI,
	Mode_Count,
};

typedef struct {
	Editor_Mode mode;

	Allocator alloc;
	Allocator frame_alloc;

	int tab_width;

	Q_Buffer *active_buffer;
	Q_Buffer *buffer_list;

	Q_Buffer *cli_buffer;
} Editor_Context;

typedef u32 Editor_Cmd_Type;
enum {
	Cmd_Mode_Change,
	Cmd_Buffer_Open,
	Cmd_Buffer_Close,

	Cmd_Insert_Text,
	Cmd_Delete_Text,
	Cmd_Cursor_Move,
	Cmd_Scroll,
};

typedef struct {
	Editor_Mode from;
	Editor_Mode to;
} Mode_Change;

typedef struct {
	String8 name;
} Buffer_New;

typedef struct {
	Q_Buffer *buffer;
} Buffer_Close;

typedef struct {
    String8 text;
} Text_Insert;

typedef struct {
    int dx;
    int dy;
} Cursor_Move;

typedef struct {
    int amount;
	bool move;
} Text_Delete;

typedef struct {
	Editor_Cmd_Type type;

	union {
		Mode_Change  mode;
		Buffer_New   buffer_open;
		Buffer_Close buffer_close;
		Text_Insert  text_insert;
		Text_Delete  text_delete;
		Cursor_Move  cursor;
	};
} Editor_Cmd;

internal Editor_Context editor_context(Allocator alloc, Allocator frame_alloc);
internal void editor_push_cmd(Editor_Context *ctx, Editor_Cmd cmd);

#endif
