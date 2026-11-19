
//////////////////////////////
// @section: linux memory hooks


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

//////////////////////////////
// @section: linux timing

function Time 
time_now(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

function void 
time_sleep_ns(Time ns)
{
  struct timespec ts = {0};
  ts.tv_sec = ns / 1000000000LL;
  ts.tv_nsec = ns % 1000000000LL; 
  nanosleep(&ts, 0);
}

//////////////////////////////
// @section: files

function void
lnx_file_info_from_stat(struct stat st, File_Info *info)
{
  info->id.v[0]   = (u64)st.st_ino;
  info->id.v[1]   = (u64)st.st_dev;
  info->size      = (s64)st.st_size;
  info->created   = (Time)st.st_ctim.tv_sec * 1000000000ull +
                     (Time)st.st_ctim.tv_nsec;
  info->modified  = (Time)st.st_mtim.tv_sec * 1000000000ull +
                     (Time)st.st_mtim.tv_nsec;

  info->flags = (!!(st.st_mode & (S_IRUSR | S_IRGRP | S_IROTH)) * File_Flag_Readable) |
                 (!!(st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) * File_Flag_Writeable);

  if      (S_ISREG(st.st_mode)) info->kind = File_Kind_Regular;
  else if (S_ISDIR(st.st_mode)) info->kind = File_Kind_Directory;
  else if (S_ISLNK(st.st_mode)) info->kind = File_Kind_Symlink;
  else                          info->kind = File_Kind_Undetermined;
}

function File_Info
file_get_info(String8 path)
{
  File_Info result = {0};
  Temp scratch = scratch_begin(0, 0);
  
  const u8 *path_cstring = str8_push_cstring(scratch.arena, path);
  
  struct stat st = {0};
  if (lstat((char *) path_cstring, &st) == 0)
  {
    lnx_file_info_from_stat(st, &result);
  }
  
  scratch_end(scratch);
  return result;
}

function s64
load_file_to_buffer(String8 path, u8 *raw, s64 cap)
{
  s64 result = 0;
  if (raw == 0)
  {
    File_Info info = file_get_info(path);
    result = info.size;
  }
  else
  {
    Temp scratch = scratch_begin(0, 0);
    const u8 *path_cstring = str8_push_cstring(scratch.arena, path);
    int fd = open((char *) path_cstring, O_RDONLY);
    if (fd >= 0)
    {
      s64 total = 0;
      while (total < cap)
      {
        s64 want = Min(cap - total, 0x7ffff000);
        ssize_t n = read(fd, raw + total, (size_t) want);
        if (n < 0)
        {
          if (errno == EINTR) continue;
          break;
        }
        if (n == 0) break;
        total += n;
      }
      close(fd);
      result = total;
    }
    scratch_end(scratch);
  }
  return result;
}

function b32 
write_file_to_path(String8 data, String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  b32 ok = false;

  const u8 *path_cstring = str8_push_cstring(scratch.arena, path);
  int fd = open((char *)path_cstring, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (fd >= 0)
  {
    s64 total = 0;
    b32 failed = false;

    while (total < data.len)
    {
      s64 want = Min(data.len - total, 0x7ffff000);
      ssize_t n = write(fd, data.raw + total, (size_t) want);
      if (n < 0)
      {
        if (errno == EINTR) continue;
        failed = true;
        break;
      }
      total += (s64)n;
    }

    // @todo(gaureesh): Apprently close() can fail? I should look this up later.
    if (close(fd) != 0) 
    {
      failed = true;
    }

    ok = !failed && (total == data.len);
  }

  scratch_end(scratch);
  return ok;
}

function b32
create_directory(String8 path)
{
  b32 ok = false;
  File_Info info = file_get_info(path);

  if (info.kind == File_Kind_DNE)
  {
    Temp scratch = scratch_begin(0, 0);

    const u8 *path_cstring = str8_push_cstring(scratch.arena, path);
    int err = mkdir((char *) path_cstring, 0777);

    scratch_end(scratch);

    ok = (err == 0);
  }
  return ok;
}

//////////////////////////////
// @section: startup

int 
main(int argc, char **argv)
{
  Thread_Ctx *tctx = tctx_alloc();
  tctx_select(tctx);
  main_thread_base_entry_point(argc, argv);
}

