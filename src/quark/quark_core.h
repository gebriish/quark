#ifndef QUARK_CORE_H
#define QUARK_CORE_H

#include "../base/base_inc.h"

#include "quark_buffer.h"

typedef u32 Quark_Cmd_Kind;
enum {
	Cmd_Kind_None,

	Cmd_Kind_New_Buffer,
	Cmd_Kind_Delete_Buffer,

	Cmd_Kind_Len,
};

typedef u32 Quark_Error;
enum {
	Quark_Err_None,
	Quark_Err_Underflow,
	Quark_Err_Overflow,
	Quark_Err_Count,
};

typedef struct Quark_Cmd Quark_Cmd;
struct Quark_Cmd {
	Quark_Cmd_Kind kind;

	union {
		struct { String8 name, data_buffer; } buffer_new;
		struct { Buffer_ID id; } buffer_delete;
	};
};

Generate_Deque(cmd_queue, Quark_Cmd)

typedef struct Quark_Ctx Quark_Ctx;
struct Quark_Ctx {
	Arena *arena;
	Arena *frame_arena;

	Arena *buffer_arena;

	cmd_queue undo_queue, redo_queue;

	QBuffer_List open_buffers;
	QBuffer_List buffer_freelist;
};

internal Quark_Ctx quark_new();
internal void quark_delete();
internal Quark_Ctx *quark_set_context(Quark_Ctx *ctx);

internal Quark_Error quark_exec_cmd(Quark_Cmd cmd);

internal Quark_Error quark_undo();
internal Quark_Error quark_redo();


#endif
