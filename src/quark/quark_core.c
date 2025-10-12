#include "quark_core.h"


internal void
quark_new(Quark_Context *context)
{
	Assert(context && "quark context ptr is null");

	context->persist_arena   = arena_alloc(MB(5));
	context->transient_arena = arena_alloc(MB(5));

	//context->buffer_manager = buffer_manager_new();
}

internal void
quark_delete(Quark_Context *context)
{
	Assert(context && "quark context ptr is null");

	arena_release(context->persist_arena);
	arena_release(context->transient_arena);
}

