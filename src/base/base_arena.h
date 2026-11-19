#ifndef BASE_ARENA_H
#define BASE_ARENA_H

typedef struct Arena Arena;
struct Arena
{
  u64 reserved;
  u64 committed;
  u64 used;
};

typedef struct Temp Temp;
struct Temp
{
  Arena *arena;
  u64    mark;
};

/////////////////////////
// @section: allocation interface

function Arena *arena_reserve(u64 reserved);
function void   arena_release(Arena *arena);
function void  *arena_alloc(Arena *arena, void *old_ptr, u64 old_size, u64 new_size, u64 alignment); 
function void   arena_free(Arena *arena, b32 decommit);

function Temp   temp_begin(Arena *arena);
function void   temp_end(Temp temp);

/////////////////////////
// @section: allocation macros

#define arena_push(_arr, _T, _c) (_T *) MemZero(arena_alloc((_arr), 0, 0, sizeof(_T) * (_c), align_of(_T)), sizeof(_T) * (_c))
#define arena_push_non_zero(_arr, _T, _c) (_T *) (arena_alloc((_arr), 0, 0, sizeof(_T) * (_c), align_of(_T)))

#define arena_push_slice(_arr, _T, _c) {  \
  .raw = arena_push(_arr, _T, _c),        \
  .len = (_c)                             \
}

#define arena_push_array(_arr, _T, _c) {  \
  .raw = arena_push(_arr, _T, _c),        \
  .capacity = (_c),                       \
  .len = 0                                \
}

#define arena_realloc_array(arena, list, type)                 \
do {                                                           \
  if ((list)->len >= (list)->capacity) {                       \
    S64 p__new_cap = Max((list)->capacity * 2, 8);             \
    (list)->raw = arena_alloc((arena), (list)->raw,            \
                              (list)->capacity * sizeof(type), \
                              p__new_cap * sizeof(type),       \
                              align_of(type));                 \
    (list)->capacity = p__new_cap;                             \
  }                                                            \
} while (0)


/////////////////////////
// @note(gaurees): OS specific hooks

function void *memory_reserve(u64 size);
function b32   memory_commit(void *ptr, u64 size);
function b32   memory_decommit(void *ptr, u64 size);
function void  memory_release(void *ptr, u64 size);

#endif // BASE_ARENA_H 
