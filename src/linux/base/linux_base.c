
//////////////////////////////
// @section: linux memory hooks


function void *
memory_reserve(U64 size)
{
  void *result = mmap(0, (size_t)size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
  if (result == MAP_FAILED)
  {
    result = 0;
  }
  return(result);
}

function B32
memory_commit(void *ptr, U64 size)
{
  return (mprotect(ptr, (size_t)size, PROT_READ|PROT_WRITE) == 0);
}

function B32
memory_decommit(void *ptr, U64 size)
{
  madvise(ptr, (size_t)size, MADV_DONTNEED);
  return (mprotect(ptr, (size_t)size, PROT_NONE) == 0);
}

function void 
memory_release(void *ptr, U64 size)
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
lnx_file_properties_from_stat(struct stat st, FileProperty *props)
{
  props->id.u64[0] = (U64)st.st_ino;
  props->id.u64[1] = (U64)st.st_dev;
  props->size      = (S64)st.st_size;
  props->created   = (Time)st.st_ctim.tv_sec * 1000000000ull +
                     (Time)st.st_ctim.tv_nsec;
  props->modified  = (Time)st.st_mtim.tv_sec * 1000000000ull +
                     (Time)st.st_mtim.tv_nsec;

  props->flags = (!!(st.st_mode & (S_IRUSR | S_IRGRP | S_IROTH)) * FileFlag_Readable) |
                 (!!(st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) * FileFlag_Writeable);

  if      (S_ISREG(st.st_mode)) props->kind = FileKind_Regular;
  else if (S_ISDIR(st.st_mode)) props->kind = FileKind_Directory;
  else if (S_ISLNK(st.st_mode)) props->kind = FileKind_Symlink;
  else                          props->kind = FileKind_Undetermined;
}

function FileProperty 
file_property(String8 path)
{
  FileProperty result = {0};
  Temp scratch = scratch_begin(0, 0);
  
  const U8 *path_cstring = str8_push_cstring(scratch.arena, path);
  
  struct stat st = {0};
  if (lstat((char *) path_cstring, &st) == 0)
  {
    lnx_file_properties_from_stat(st, &result);
  }
  
  scratch_end(scratch);
  return result;
}

function S64 
load_file_to_buffer(String8 path, U8 *raw, S64 cap)
{
  S64 result = 0;
  if (raw == 0)
  {
    FileProperty props = file_property(path);
    result = props.size;
  }
  else
  {
    Temp scratch = scratch_begin(0, 0);
    const U8 *path_cstring = str8_push_cstring(scratch.arena, path);
    int fd = open((char *) path_cstring, O_RDONLY);
    if (fd >= 0)
    {
      S64 total = 0;
      while (total < cap)
      {
        S64 want = Min(cap - total, 0x7ffff000);
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

function B32 
write_file_to_path(String8 data, String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32 ok = false;

  const U8 *path_cstring = str8_push_cstring(scratch.arena, path);
  int fd = open((char *)path_cstring, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (fd >= 0)
  {
    S64 total = 0;
    B32 failed = false;

    while (total < data.len)
    {
      S64 want = Min(data.len - total, 0x7ffff000);
      ssize_t n = write(fd, data.raw + total, (size_t) want);
      if (n < 0)
      {
        if (errno == EINTR) continue;
        failed = true;
        break;
      }
      total += (S64)n;
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

function B32
create_directory(String8 path)
{
  B32 ok = false;
  FileProperty prop = file_property(path);

  if (prop.kind == FileKind_DNE)
  {
    Temp scratch = scratch_begin(0, 0);

    const U8 *path_cstring = str8_push_cstring(scratch.arena, path);
    int err = mkdir((char *) path_cstring, 0777);

    scratch_end(scratch);

    ok = (err == 0);
  }
  return ok;
}

//////////////////////////////
// @section: threading

function S32 
run_shell_command(String8List args)
{
  Temp scratch = scratch_begin(0, 0);
  String8 arg_string = str8_list_join(scratch.arena, &args, ' ');

  pid_t pid = fork();
  if (pid == 0)
  {
    execl("/bin/sh", "sh", "-c", (char *) arg_string.raw, (char *) 0);
    _exit(127);
  }

  int status;
  waitpid(pid, &status, 0);
  scratch_end(scratch);

  return (S32) status;
}

//////////////////////////////
// @section: startup

int 
main(int argc, char **argv)
{
  ThreadCtx *tctx = tctx_alloc();
  tctx_select(tctx);
  main_thread_base_entry_point(argc, argv);
}

