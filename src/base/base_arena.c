
const U64 COMMIT_CHUNK_SIZE = Kilobytes(16);

function Arena *
arena_reserve(U64 reserved)
{
  assert(sizeof(Arena) < reserved);

  Arena *arena = 0;

  void *mem = memory_reserve(reserved);
  U64 commit = Min(reserved, COMMIT_CHUNK_SIZE);
  memory_commit(mem, commit);

  arena = (Arena *) mem;
  arena->used = sizeof(Arena);
  arena->reserved = reserved;
  arena->committed = commit;
  // AsanPoison(mem + arena->used, arena->committed - arena->used);

  return arena;
}

function void
arena_release(Arena *arena)
{
  if (arena) memory_release(arena, arena->committed);
}


function void *
arena_alloc(Arena *arena, void *old_ptr, U64 old_size, U64 new_size, U64 alignment)
{
  if (!arena)
  {
    return 0;
  }

  void *result = 0;
  U8   *mem    = (U8 *)arena;

  if (old_ptr == 0)
  {
    U64 start = align_up_power2(arena->used, alignment);
    U64 end = start + new_size;

    if (end <= arena->reserved)
    {
      if (end > arena->committed)
      {
        U64 new_commit = align_up_power2(end, COMMIT_CHUNK_SIZE);
        new_commit = Min(new_commit, arena->reserved);

        memory_commit(mem + arena->committed, new_commit - arena->committed);
        arena->committed = new_commit;
      }

      result = mem + start;
      arena->used = end;

      // AsanUnpoison(result, new_size);
      // AsanPoison(mem + arena->used, arena->committed - arena->used);
    }
  }
  else if ((U8 *)old_ptr + old_size == mem + arena->used)
  {
    U64 start = (U64)((U8 *)old_ptr - mem);
    U64 end   = start + new_size;

    if (end <= arena->reserved)
    {
      if (end > arena->committed)
      {
        U64 new_commit = align_up_power2(end, COMMIT_CHUNK_SIZE);
        new_commit = Min(new_commit, arena->reserved);
        memory_commit(mem + arena->committed, new_commit - arena->committed);
        arena->committed = new_commit;
      }

      arena->used = end;
      result = old_ptr;
      // AsanUnpoison(result, new_size);
      // AsanPoison(mem + arena->used, arena->committed - arena->used);
    }
  }
  else
  {
    U64 start = (U64)align_up_power2((U64)arena->used, (U64)alignment);
    U64 end = start + new_size;

    if (end <= arena->reserved)
    {
      if (end > arena->committed)
      {
        U64 new_commit = align_up_power2(end, COMMIT_CHUNK_SIZE);
        new_commit = Min(new_commit, arena->reserved);
        memory_commit(mem + arena->committed, new_commit - arena->committed);
        arena->committed = new_commit;
      }

      result = mem + start;
      arena->used = end;

      U64 copy_size = Min(old_size, new_size);
      MemMove(result, old_ptr, copy_size);

      // AsanUnpoison(result, new_size);
      // AsanPoison((U8 *)old_ptr, old_size);
      // AsanPoison(mem + arena->used, arena->committed - arena->used);
    }
  }

  return(result);
}

function void
arena_free(Arena *arena, B32 decommit)
{
  // AsanPoison((U8 *)arena + sizeof(Arena), arena->used - sizeof(Arena));
  arena->used = sizeof(Arena);

  U64 keep = align_up_power2(arena->used, COMMIT_CHUNK_SIZE);
  if (decommit)
  {
    memory_decommit((U8 *)arena + keep, arena->committed - keep);
    arena->committed = keep;
  }
}

function Temp
temp_begin(Arena *arena)
{
  Temp result = {0};
  result.arena = arena;
  result.mark = arena->used;
  return result;
}

function void
temp_end(Temp temp)
{
  if (temp.mark < temp.arena->used)
  {
    // AsanPoison((U8 *)temp.arena + temp.mark, temp.arena->used - temp.mark);
    temp.arena->used = temp.mark;
  }
}
