
const u64 COMMIT_CHUNK_SIZE = Kilobytes(16);

function Arena *
arena_reserve(u64 reserved)
{
  assert(sizeof(Arena) < reserved);

  Arena *arena = 0;

  void *mem = memory_reserve(reserved);
  u64 commit = Min(reserved, COMMIT_CHUNK_SIZE);
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
arena_alloc(Arena *arena, void *old_ptr, u64 old_size, u64 new_size, u64 alignment)
{
  if (!arena)
  {
    return 0;
  }

  void *result = 0;
  u8   *mem    = (u8 *)arena;

  if (old_ptr == 0)
  {
    u64 start = align_up_power2(arena->used, alignment);
    u64 end = start + new_size;

    if (end <= arena->reserved)
    {
      if (end > arena->committed)
      {
        u64 new_commit = align_up_power2(end, COMMIT_CHUNK_SIZE);
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
  else if ((u8 *)old_ptr + old_size == mem + arena->used)
  {
    u64 start = (u64)((u8 *)old_ptr - mem);
    u64 end   = start + new_size;

    if (end <= arena->reserved)
    {
      if (end > arena->committed)
      {
        u64 new_commit = align_up_power2(end, COMMIT_CHUNK_SIZE);
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
    u64 start = (u64)align_up_power2((u64)arena->used, (u64)alignment);
    u64 end = start + new_size;

    if (end <= arena->reserved)
    {
      if (end > arena->committed)
      {
        u64 new_commit = align_up_power2(end, COMMIT_CHUNK_SIZE);
        new_commit = Min(new_commit, arena->reserved);
        memory_commit(mem + arena->committed, new_commit - arena->committed);
        arena->committed = new_commit;
      }

      result = mem + start;
      arena->used = end;

      u64 copy_size = Min(old_size, new_size);
      MemMove(result, old_ptr, copy_size);

      // AsanUnpoison(result, new_size);
      // AsanPoison((U8 *)old_ptr, old_size);
      // AsanPoison(mem + arena->used, arena->committed - arena->used);
    }
  }

  return(result);
}

function void
arena_free(Arena *arena, b32 decommit)
{
  // AsanPoison((U8 *)arena + sizeof(Arena), arena->used - sizeof(Arena));
  arena->used = sizeof(Arena);

  u64 keep = align_up_power2(arena->used, COMMIT_CHUNK_SIZE);
  if (decommit)
  {
    memory_decommit((u8 *)arena + keep, arena->committed - keep);
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
