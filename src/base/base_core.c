#include "base_core.h"

#include <stdlib.h>

///////////////////////////////////////////
// ~geb: General Perpose heap allocator

// global heap allocator functions
#define HeapRealloc realloc
#define HeapAlloc   malloc
#define HeapFree    free

internal void
heap_aligned_free(void *ptr) 
{
	if (ptr) {
		void *original_alloc = ((void**)ptr)[-1];
		if (original_alloc) {
			HeapFree(original_alloc);
		}
	}
}

internal Alloc_Result
heap_aligned_alloc(usize size, usize alignment, void *old_ptr, usize old_size, bool zero_mem)
{
	usize align = Max(alignment, (usize)AlignOf(void*));
	usize space;
	if (align == 0) align = AlignOf(void*);
	space = (align - 1) + sizeof(void*) + size;

	void *allocated_mem = NULL;
	bool force_copy = (old_ptr != NULL) && (alignment > (usize)AlignOf(void*));

	if (old_ptr != NULL && !force_copy) {
		void *original_old_alloc = ((void**)old_ptr)[-1];
		allocated_mem = HeapRealloc(original_old_alloc, space);
	} else {
		allocated_mem = HeapAlloc(space);
		if (allocated_mem != NULL && zero_mem) {
			MemZero(allocated_mem, space);
		}
	}

	if (allocated_mem == NULL) {
		Alloc_Result result = { NULL, 0, Alloc_Err_Out_Of_Memory };
		return result;
	}

	uintptr_t base = (uintptr_t)allocated_mem + sizeof(void*);
	uintptr_t aligned = (base + (align - 1)) & ~((uintptr_t)(align - 1));
	void *user_ptr = (void*)aligned;

	((void**)user_ptr)[-1] = allocated_mem;

	if (force_copy) {
		usize to_copy = Min(old_size, size);
		if (to_copy > 0) {
			MemMove(user_ptr, old_ptr, to_copy);
		}
		void *old_original_alloc = ((void**)old_ptr)[-1];
		if (old_original_alloc) {
			HeapFree(old_original_alloc);
		}
	}

	Alloc_Result result = { user_ptr, size, Alloc_Err_None };
	return result;
}


internal Alloc_Result
heap_aligned_resize(void *ptr, usize old_size, usize new_size, usize alignment, bool zero) 
{
	if (ptr == NULL) {
		return heap_aligned_alloc(new_size, alignment, NULL, 0, zero);
	}

	Alloc_Result result = heap_aligned_alloc(new_size, alignment, ptr, old_size, zero);
	if (result.err != Alloc_Err_None) {
		return result;
	}

	if (zero && new_size > old_size && result.mem != NULL) {
		void *start = (u8 *)result.mem + old_size;
		MemZero(start, new_size - old_size);
	}

	return result;
}

internal Alloc_Result
heap_allocator_proc(
	void *allocator_data,
	Alloc_Mode mode,
	usize size,
	usize alignment,
	void *old_memory,
	usize old_size,
	Source_Code_Location loc
)
{
	switch (mode) {
		case AlMode_Alloc: case AlMode_Alloc_Non_Zeroed:
			return heap_aligned_alloc(size, alignment, NULL, 0, mode == AlMode_Alloc);
			break;

		case AlMode_Free:
			heap_aligned_free(old_memory);
			break;
		case AlMode_Free_All: {
			Alloc_Result result = {NULL, 0 , Alloc_Err_Mode_Not_Implemented};
			return result;
		} break;

		case AlMode_Resize: case AlMode_Resize_Non_Zeroed:
			return heap_aligned_resize(old_memory, old_size, size, alignment, mode == AlMode_Resize);
			break;
	}

	return (Alloc_Result){};	
}


internal Allocator 
gp_heap_allocator()
{
	Allocator heap = {
		.procedure = heap_allocator_proc,
		.data = NULL,
	};
	return heap;
}

///////////////////////////////////////////
// ~geb: Temporary Arena allocator

typedef struct Memory_Block Memory_Block;
struct Memory_Block {
	Memory_Block *prev;
	Allocator allocator;
	u8 *base;
	usize used, capacity;
};

typedef struct _Arena _Arena;
struct _Arena {
	Allocator backing_allocator;
	Memory_Block *current_block;
	usize total_used;
	usize total_capacity;
	usize min_block_size;
	usize temp_count;
};

internal Allocator
arena_allocator() 
{
	Allocator arena = {
		.data = NULL,
	};
	return arena;
}

///////////////////////////////////////////
// ~geb: Arena Allocator

internal Arena *
arena_new(u8 *mem, usize capacity)
{
	void *base = mem;

	Arena *arena = (Arena *) base;
	arena->last_used = ARENA_HEADER_SIZE;
	arena->used = ARENA_HEADER_SIZE;
	arena->capacity = capacity;
	arena->nested = false;
	return arena;
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

	/*
#if DEBUG_BUILD
	printf("arena_push %6zu bytes%s at %s:%d (%s)\n",
		params->size,
		params->zero ? " [zero]" : "",
		params->caller_file,
		params->caller_line,
		params->caller_proc);
#endif
	*/

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

