#ifndef QUARK_CORE_H
#define QUARK_CORE_H

#include "../base/base_core.h"
#include "../gfx/gfx_core.h"

#include "quark_buffer.h"
#include "quark_window.h"

typedef u32	Quark_State;
enum {
	Quark_State_Normal,
	Quark_State_Insert,
	Quark_State_Cmd,
};

typedef struct Quark_Context Quark_Context;
struct Quark_Context {
	Arena *persist_arena;
	Arena *transient_arena;

	Quark_State state;
	Buffer_Manager buffer_manager;

	Gap_Buffer *cmd_gap_buffer;
};

internal void quark_new(Quark_Context *context);
internal void quark_delete(Quark_Context *context);

internal void quark_frame_update(Quark_Context *context, Input_Data data);

#endif
