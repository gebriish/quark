#include "ui_core.h"


global UI_Context *g_ui_context;


internal UI_Context *
ui_init(Arena *allocator)
{
	UI_Context *ctx = arena_push_struct(allocator, UI_Context);
	ctx->internal_arena = arena_nest(allocator, KB(512));

	
}

internal UI_Context * 
ui_set_context(UI_Context *ctx)
{
	UI_Context *prev = g_ui_context;
	g_ui_context = ctx;
	return prev;
}
