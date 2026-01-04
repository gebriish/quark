#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#include "base_core.h"

#define COMMIT_BLOCK_SIZE KB(64)

typedef void* (*MemReserveFn)(usize size);
typedef int   (*MemCommitFn)(void* ptr, usize size);
typedef void  (*MemDecommitFn)(void* ptr, usize size);
typedef void  (*MemReleaseFn)(void* ptr, usize size);

typedef struct Arena Arena;
struct Arena {
	void* base;
	usize size;
	usize committed;
	usize pos;

	MemReserveFn  reserve;
	MemCommitFn   commit;
	MemDecommitFn decommit;
	MemReleaseFn  release;
};

internal Arena* arena_new(
	usize reserve_size,
	MemReserveFn reserve,
	MemCommitFn commit,
	MemDecommitFn decommit,
	MemReleaseFn release
);
internal void   arena_delete(Arena* arena);

internal void*  arena_alloc(Arena* arena, usize size, usize align, Source_Code_Location loc);
internal void*  arena_realloc(Arena *arena, void *ptr, usize new_size, usize old_size);
internal void   arena_reset(Arena* arena); // just pointer backtrack
internal void   arena_clear(Arena* arena); // decommits backing mem back to os

internal void   arena_pop_to(Arena *arena, usize pos);

#define arena_push(a, T) (T *)arena_alloc((a), sizeof(T), AlignOf(T), Code_Location)
#define arena_push_array(a, T, n) (T *)arena_alloc((a), sizeof(T) * (n), AlignOf(T), Code_Location)

typedef struct Arena_Scope Arena_Scope;
struct Arena_Scope {
	Arena *arena;
	usize pos;
};

internal Arena_Scope arena_scope_begin(Arena *arena);
internal void        arena_scope_end(Arena_Scope scope);



#endif
