////////////////////////////////
// ~geb: Arena Allocator 

#include "base_core.h"

#include <stdlib.h>

internal_lnk Arena *
arena_alloc(usize capacity)
{
  void *base = malloc(capacity);

  Arena *arena = (Arena *) base;
  arena->last_used = ARENA_HEADER_SIZE;
  arena->used = ARENA_HEADER_SIZE;
  arena->capacity = capacity;
  return arena;
}

internal_lnk void
arena_release(Arena *arena)
{
  Assert(arena && "trying to release null arena");
  free(arena);
}

internal_lnk void *
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
  printf("[arena] alloc %6zu bytes%s -> %p\n"
         "        at %s:%d (%s)\n",
         params->size,
         params->zero ? " [zero]" : "",
         result,
         params->caller_file,
         params->caller_line,
         params->caller_proc);
#endif

  return result;
}

internal_lnk usize
arena_pos(Arena *arena)
{
  return arena->used;
}

internal_lnk void 
arena_pop(Arena *arena)
{
  arena->used = arena->last_used;
}

internal_lnk void
arena_pop_to(Arena *arena, usize pos)
{
  Assert(pos >= ARENA_HEADER_SIZE && "Cannot pop into header");
  arena->last_used = pos;
  arena->used = pos;
}

internal_lnk void 
arena_clear(Arena *arena)
{
  arena->used = ARENA_HEADER_SIZE;
  arena->last_used = ARENA_HEADER_SIZE;
}

internal_lnk Temp 
temp_begin(Arena *arena)
{
  Temp result = {
    .arena = arena,
    .pos = arena->used
  };
  return result;
}

internal_lnk void   
temp_end(Temp temp)
{
  Arena *arena = temp.arena;
  arena->used = temp.pos;
}

