#ifndef QUARK_CORE_H
#define QUARK_CORE_H

#include "../base/base_inc.h"
#include "../gfx/gfx_inc.h"

#include "quark_buffer.h"


Enum(Quark_Cmd_Kind, u32) {
	Cmd_Kind_None,

	Cmd_Kind_New_Buffer,
	Cmd_Kind_Delete_Buffer,
	Cmd_Kind_Save_Buffer,
	Cmd_Kind_Open_File,

	Cmd_Kind_State_Change,

	Cmd_Kind_Reset,
	Cmd_Kind_Count,
};

Enum(Quark_State, u32) {
	State_Normal,
	State_Insert,
	State_Visual,
	State_Command,
};

Enum(Quark_Error, u32) {
	Quark_Err_None,
	Quark_Err_Underflow,
	Quark_Err_Overflow,
	Quark_Err_File_Save_Failed,
	Quark_Err_Count,
};

typedef struct Quark_Cmd Quark_Cmd;
struct Quark_Cmd {
	Quark_Cmd_Kind kind;

	union {
		struct { String8 name, data_buffer; } buffer_new;
		struct { QBuffer *buf; } buffer_delete;
		struct { QBuffer *buf; } save_buffer;
		struct { String8 path; } open_file;
		Quark_State to_state;
	};
};

Generate_Deque(cmd_queue, Quark_Cmd)

typedef struct Quark_Ctx Quark_Ctx;
struct Quark_Ctx {
	Quark_State state;

	Arena *arena;
	Arena *frame_arena;
	Arena *buffer_arena;

	cmd_queue undo_queue, redo_queue;

	QBuffer_List open_buffers;
	QBuffer_List buffer_freelist;

	QBuffer *active_buffer;
};

internal Quark_Ctx quark_new();
internal void quark_delete();
internal Quark_Ctx *quark_set_context(Quark_Ctx *ctx);

internal void quark_update_state(Input_Data *data);

internal Quark_Error quark_exec_cmd(Quark_Cmd cmd);

internal Quark_Error quark_undo();
internal Quark_Error quark_redo();

#endif
