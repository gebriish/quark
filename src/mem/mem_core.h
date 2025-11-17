#ifndef MEM_CORE_H
#define MEM_CORE_H

#include "../base/base_core.h"

////////////////////////////////
// ~geb: Allocator Interface 

typedef u32 Alloc_Error;
enum {
	Alloc_Err_None,
	Alloc_Err_Out_Of_Memory,
	Alloc_Err_Invalid_Pointer,
	Alloc_Err_Invalid_Argument,
	Alloc_Err_Mode_Not_Implemented,
};


typedef u32 Alloc_Mode;
enum {
	AlMode_Alloc,
	AlMode_Free,
	AlMode_Free_All,
	AlMode_Resize,
	AlMode_Alloc_Non_Zeroed,
	AlMode_Resize_Non_Zeroed,
};

typedef struct Alloc_Result Alloc_Result;
struct Alloc_Result {
	void *mem;
	Alloc_Error err;
};

typedef Alloc_Result (*Alloc_Proc)(
	void *allocator_data,
	Alloc_Mode mode,
	usize size,
	usize alignment,
	void *old_memory,
	usize old_size
#if DEBUG_BUILD
	, Source_Code_Location loc
#endif
);

typedef struct Allocator Allocator;
struct Allocator {
	Alloc_Proc alloc_proc;
	void *mem;
};

////////////////////////////////
// ~geb: Arena Allocator 

#define ARENA_HEADER_SIZE sizeof(Arena)

typedef struct Arena Arena;
struct Arena {
	usize last_used;
	usize used;
	usize capacity;
	bool  nested;
};

typedef struct Alloc_Params Alloc_Params;
struct Alloc_Params {
	usize size;
	usize align;
	bool zero;
#if DEBUG_BUILD
	const char *caller_proc;
	const char *caller_file;
	int         caller_line;
#endif
};

typedef struct Temp Temp;
struct Temp {
	Arena *arena;
	usize pos;
};

internal Arena *arena_new(u8 *mem, usize capacity);

internal void  *arena_push_(Arena *arena, Alloc_Params *params);
internal void   arena_pop(Arena *arena);
internal void   arena_pop_to(Arena *arena, usize pos);
internal void   arena_clear(Arena *arena);
internal usize  arena_pos(Arena *arena);
internal void   arena_print_usage(Arena *arena, const char *name);

internal Temp   temp_begin(Arena *arena);
internal void   temp_end(Temp temp);

#define arena_push_struct(a, T)   (T *) arena_push((a), sizeof(T), AlignOf(T), true)
#define arena_push_array_zeroed(a, T, c) (T *) arena_push((a), sizeof(T) * (c), AlignOf(T), true)
#define arena_push_array(a, T, c) (T *) arena_push((a), sizeof(T) * (c), AlignOf(T), false)

#if DEBUG_BUILD
#define _ARENA_DEBUG_FIELDS_ , .caller_proc = __func__, .caller_file = __FILE__, .caller_line = __LINE__
#else
#define _ARENA_DEBUG_FIELDS_
#endif

#define arena_push(ar, s, al, zr)                                     \
arena_push_(ar, &(Alloc_Params){                                      \
	.size  = s,                                                         \
	.align = al,                                                        \
	.zero  = zr                                                         \
	_ARENA_DEBUG_FIELDS_                                                \
})

#endif
