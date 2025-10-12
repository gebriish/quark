///////////////////////////////
// ~geb: Arena Allocator 

#include "base_core.h"

#include <stdlib.h>

internal Arena *
arena_alloc(usize capacity)
{
	void *base = malloc(capacity);

	Arena *arena = (Arena *) base;
	arena->last_used = ARENA_HEADER_SIZE;
	arena->used = ARENA_HEADER_SIZE;
	arena->capacity = capacity;
	return arena;
}

internal Arena *arena_realloc_(Arena *arena, usize new_capacity)
{
	if (!arena || arena->capacity >= new_capacity) return arena;


	u8 *base = malloc(new_capacity);
	if (!base) {
		LogError("Arena realloc Failed");
		return arena;
	}

	MemCopy(base, arena, arena->used);

	Arena *new_arena = (Arena *)base;
	new_arena->capacity = new_capacity;

	free(arena);

	return new_arena;
}

internal void
arena_release(Arena *arena)
{
	Assert(arena && "trying to release null arena");
	free(arena);
}

internal void *
arena_push_(Arena *arena, Alloc_Params *params)
{
	usize used_pre = AlignPow2(arena->used, params->align);
	usize used_pst = used_pre + params->size;

	AssertAlways(used_pst <= arena->capacity && "Arena ran out of memory");

	arena->last_used = arena->used;

	void *result = (u8 *)arena + used_pre;
	arena->used = used_pst;

	if (params->zero) {
		MemZero(result, params->size);
	}

#if DEBUG_BUILD
	printf("arena_push %6zu bytes%s at %s:%d (%s)\n",
		params->size,
		params->zero ? " [zero]" : "",
		params->caller_file,
		params->caller_line,
		params->caller_proc);
#endif

	return result;
}

internal usize
arena_pos(Arena *arena)
{
	return arena->used;
}

internal void 
arena_pop(Arena *arena)
{
	arena->used = arena->last_used;
}

internal void
arena_pop_to(Arena *arena, usize pos)
{
	Assert(pos >= ARENA_HEADER_SIZE && "Cannot pop into header");
	arena->last_used = pos;
	arena->used = pos;
}

internal void 
arena_clear(Arena *arena)
{
	arena->used = ARENA_HEADER_SIZE;
	arena->last_used = ARENA_HEADER_SIZE;
}

internal Temp 
temp_begin(Arena *arena)
{
	Temp result = {
		.arena = arena,
		.pos = arena->used
	};
	return result;
}

internal void   
temp_end(Temp temp)
{
	Arena *arena = temp.arena;
	arena->used = temp.pos;
}


internal void
arena_print_usage(Arena *arena, const char *name)
{
#if DEBUG_BUILD
	if (!arena) {
		printf("Arena(%s): NULL\n", name ? name : "unnamed");
		return;
	}

	f64 used_pct = 0;
	if (arena->capacity > 0) {
		used_pct = ((f64)arena->used / (f64)arena->capacity) * 100.0;
	}

	printf("Arena(%s):\n", name ? name : "unnamed");
	printf("  used:       %zu bytes\n", arena->used);
	printf("  last_used:  %zu bytes\n", arena->last_used);
	printf("  capacity:   %zu bytes\n", arena->capacity);
	printf("  usage:      %.2f%%\n", used_pct);
#endif
}

