
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

function String8
file_name_from_path(String8 path)
{
  String8 result = path;
  s64 i = path.len;
  for(; i > 0; i -= 1)
  {
    if(path.raw[i - 1] == '/')
    {
      break;
    }
  }
  result.raw += i;
  result.len -= i;
  return(result);
}

///////////////////////////////
// @section: paths

function b32 
set_current_working_directory(String8 path)
{
  b32 success = false;
  Temp scratch = scratch_begin(0, 0);
  const char *path_cstring = (char *) str8_push_cstring(scratch.arena, path);
  success = (chdir(path_cstring) == 0);
  scratch_end(scratch);
  return success;
}

function String8 
get_home_directory(void)
{
  char *home = getenv("HOME");
  if(home == 0 || home[0] == 0)
  {
    struct passwd *pw = getpwuid(getuid());
    if(pw != 0)
    {
      home = pw->pw_dir;
    }
  }
  return str8_cstring((u8 *) home);
}

function String8
get_absolute_directory(Arena *arena, String8 rel)
{
  Temp   scratch = scratch_begin(&arena, 1);
  String8 result = {0};

  const char *path = (char *) str8_push_cstring(scratch.arena, rel);
  char *absolute = realpath( path, 0);

  if(absolute != 0)
  {
    result = str8_push_copy(arena, str8_cstring((u8 *) absolute));
    free(absolute);
  }

  scratch_end(scratch);
  return result;
}

function String8
get_current_working_directory(Arena *arena)
{
  String8 result = {0};

  char *cwd = getcwd(0, 0);
  if(cwd != 0)
  {
    result = str8_push_copy(arena, str8_cstring((u8 *)cwd));
    free(cwd);
  }

  return result;
}
