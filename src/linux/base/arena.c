
function void *
memory_reserve(u64 size)
{
  void *result = mmap(0, (size_t)size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
  if (result == MAP_FAILED)
  {
    result = 0;
  }
  return(result);
}

function b32
memory_commit(void *ptr, u64 size)
{
  return (mprotect(ptr, (size_t)size, PROT_READ|PROT_WRITE) == 0);
}

function b32
memory_decommit(void *ptr, u64 size)
{
  madvise(ptr, (size_t)size, MADV_DONTNEED);
  return (mprotect(ptr, (size_t)size, PROT_NONE) == 0);
}

function void 
memory_release(void *ptr, u64 size)
{
  munmap(ptr, (size_t)size);
}
