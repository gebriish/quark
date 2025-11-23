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

internal Alloc_Buffer
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
		Alloc_Buffer result = { NULL, 0, Alloc_Err_Out_Of_Memory };
		return result;
	}

	usize base = (usize)allocated_mem + sizeof(void*);
	usize aligned = AlignPow2(base, align);
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

	Alloc_Buffer result = { user_ptr, size, Alloc_Err_None };
	return result;
}


internal Alloc_Buffer
heap_aligned_resize(void *ptr, usize old_size, usize new_size, usize alignment, bool zero) 
{
	if (ptr == NULL) {
		return heap_aligned_alloc(new_size, alignment, NULL, 0, zero);
	}

	Alloc_Buffer result = heap_aligned_alloc(new_size, alignment, ptr, old_size, zero);
	if (result.err != Alloc_Err_None) {
		return result;
	}

	if (zero && new_size > old_size && result.mem != NULL) {
		void *start = (u8 *)result.mem + old_size;
		MemZero(start, new_size - old_size);
	}

	return result;
}

internal Alloc_Buffer
heap_allocator_proc(
	void *allocator_data,
	Alloc_Mode mode,
	usize size,
	usize alignment,
	void *old_memory,
	usize old_size,
	Source_Code_Location loc
) {
	switch (mode) {
		case AlMode_Alloc: case AlMode_Alloc_Non_Zeroed:
			return heap_aligned_alloc(size, alignment, NULL, 0, mode == AlMode_Alloc);
			break;

		case AlMode_Free:
			heap_aligned_free(old_memory);
			break;
		case AlMode_Free_All: {
			Alloc_Buffer result = {NULL, 0 , Alloc_Err_Mode_Not_Implemented};
			return result;
		} break;

		case AlMode_Resize: case AlMode_Resize_Non_Zeroed:
			return heap_aligned_resize(old_memory, old_size, size, alignment, mode == AlMode_Resize);
			break;
	}

	return (Alloc_Buffer){};	
}

internal Allocator 
heap_allocator()
{
	Allocator heap = {
		.procedure = heap_allocator_proc,
		.data = NULL,
	};
	return heap;
}

///////////////////////////////////////////
// ~geb: Temporary Arena allocator


internal Arena *
arena_new(Alloc_Buffer backing_buffer) {
	Assert(backing_buffer.err == Alloc_Err_None);
	Assert(backing_buffer.size > ARENA_HEADER_SIZE * 2);

	Arena *result = (Arena *)backing_buffer.mem;

	result->used = ARENA_HEADER_SIZE;
	result->capacity = backing_buffer.size;

	return result;
}

internal Alloc_Buffer
arena_aligned_alloc(Arena *arena, usize size, usize alignment, bool zero_mem)
{
	Assert(arena);
	if (size == 0) {
		Alloc_Buffer z = {NULL, 0, Alloc_Err_None};
		return z;
	}

	usize align = Max(alignment, (usize)AlignOf(void*));
	if (align == 0) align = AlignOf(void*);

	usize arena_base = (usize)arena;
	usize current_used = arena->used;
	usize base_addr = arena_base + current_used + sizeof(void*);
	usize aligned_addr = AlignPow2(base_addr, align);
	usize offset = aligned_addr - arena_base;

	if (offset + size > arena->capacity) {
		Alloc_Buffer result = { NULL, 0, Alloc_Err_Out_Of_Memory };
		return result;
	}

	void *user_ptr = (void*)aligned_addr;
	arena->used = offset + size;

	if (zero_mem) {
		MemZero(user_ptr, size);
	}

	Alloc_Buffer result = { user_ptr, size, Alloc_Err_None };
	return result;
}

internal Alloc_Buffer
arena_aligned_resize(Arena *arena, void *old_memory, usize old_size, usize new_size, usize alignment, bool zero_mem)
{
	Assert(arena);

	if (old_memory == NULL) {
		return arena_aligned_alloc(arena, new_size, alignment, zero_mem);
	}

	usize arena_base = (usize)arena;
	usize ptr_addr = (usize)old_memory;
	if (ptr_addr < arena_base + ARENA_HEADER_SIZE || ptr_addr >= arena_base + arena->used) {
		Alloc_Buffer fail = { NULL, 0, Alloc_Err_Mode_Not_Implemented };
		return fail;
	}

	usize old_offset = ptr_addr - arena_base;
	usize old_end = old_offset + old_size;
	usize current_used = arena->used;

	if (old_end == current_used) {
		usize desired_end = old_offset + new_size;
		if (desired_end <= arena->capacity) {
			arena->used = desired_end;
			if (zero_mem && new_size > old_size) {
				void *start = (u8*)old_memory + old_size;
				MemZero(start, new_size - old_size);
			}
			Alloc_Buffer ok = { old_memory, new_size, Alloc_Err_None };
			return ok;
		} else {
			Alloc_Buffer fail = { NULL, 0, Alloc_Err_Out_Of_Memory };
			return fail;
		}
	} else {
		if (new_size <= old_size) {
			Alloc_Buffer ok = { old_memory, new_size, Alloc_Err_None };
			return ok;
		} else {
			Alloc_Buffer fail = { NULL, 0, Alloc_Err_Mode_Not_Implemented };
			return fail;
		}
	}
}

internal Alloc_Buffer
arena_allocator_proc(
	void *allocator_data,
	Alloc_Mode mode,
	usize size,
	usize alignment,
	void *old_memory,
	usize old_size,
	Source_Code_Location loc
) {
	Arena *arena = (Arena *)allocator_data;
	Assert(arena);

	switch (mode) {
		case AlMode_Alloc: case AlMode_Alloc_Non_Zeroed:
			return arena_aligned_alloc(arena, size, alignment, mode == AlMode_Alloc);
		case AlMode_Free: 
			return (Alloc_Buffer){ NULL, 0, Alloc_Err_Mode_Not_Implemented };

		case AlMode_Free_All:
			arena->used = ARENA_HEADER_SIZE;
			return (Alloc_Buffer) { NULL, 0, Alloc_Err_None };

		case AlMode_Resize: case AlMode_Resize_Non_Zeroed:
			return arena_aligned_resize(arena, old_memory, old_size, size, alignment, mode == AlMode_Resize);
	}
	return (Alloc_Buffer) {};
}

internal Allocator
arena_allocator(Arena *arena) 
{
	Assert(arena);

	Allocator alloc = {
		.procedure = arena_allocator_proc,
		.data = arena,
	};
	return alloc;
}
