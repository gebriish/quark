#include "../os_core.h"
#include "os_core_linux.h"

internal void * os_reserve(usize size)
{
	void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(result == MAP_FAILED)
	{
		result = 0;
	}
	return result;
}

internal int
os_commit(void *ptr, usize size)
{
	return mprotect(ptr, size, PROT_READ | PROT_WRITE);
}

internal void
os_decommit(void *ptr, usize size)
{
	madvise(ptr, size, MADV_DONTNEED);
	mprotect(ptr, size, PROT_NONE);
}

internal void
os_release(void *ptr, usize size)
{
  munmap(ptr, size);
}


internal OS_Handle
os_file_open(OS_Access_Flags flags, String8 path, Arena *scratch)
{
	Arena_Scope scope = arena_scope_begin(scratch);

  String8 path_cstring = str8_concat(path, S("\0"), scratch);

  int linx_flags = 0;
  if(flags & OS_AccessFlag_Read && flags & OS_AccessFlag_Write)
  {
    linx_flags = O_RDWR;
  }
  else if(flags & OS_AccessFlag_Write)
  {
    linx_flags = O_WRONLY;
  }
  else if(flags & OS_AccessFlag_Read)
  {
    linx_flags = O_RDONLY;
  }
  if(flags & OS_AccessFlag_Append)
  {
    linx_flags |= O_APPEND;
  }
  if(flags & (OS_AccessFlag_Write|OS_AccessFlag_Append))
  {
    linx_flags |= O_CREAT;
  }
  linx_flags |= O_CLOEXEC;
  int fd = open((char *)path_cstring.str, linx_flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  OS_Handle handle = {0};

  if(fd != -1) handle.u64 = fd;

	arena_scope_end(scope);

  return handle;
}


internal void
os_file_close(OS_Handle file)
{
  if(!file.u64) { return; }
  int fd = (int)file.u64;
  close(fd);
}

internal usize
os_file_read(OS_Handle handle, usize begin, usize end, void *out_data)
{
	if (handle.u64 == 0) { return 0; }

	int fd = (int) handle.u64;
  usize total_num_bytes_to_read = end - begin;
  usize total_num_bytes_read = 0;
  usize total_num_bytes_left_to_read = total_num_bytes_to_read;
  for(;total_num_bytes_left_to_read > 0;)
  {
    int read_result = pread(fd, (u8 *)out_data + total_num_bytes_read, total_num_bytes_left_to_read, begin + total_num_bytes_read);
    if(read_result >= 0)
    {
      total_num_bytes_read += read_result;
      total_num_bytes_left_to_read -= read_result;
    }
    else if(errno != EINTR)
    {
      break;
    }
  }
  return total_num_bytes_read;
}


internal usize
os_file_write(OS_Handle file, usize begin, usize end, u8 *data)
{
  if(file.u64 == 0) { return 0; }
  int fd = (int) file.u64;
  usize total_num_bytes_to_write = end - begin;
  usize total_num_bytes_written = 0;
  usize total_num_bytes_left_to_write = total_num_bytes_to_write;
  for(;total_num_bytes_left_to_write > 0;)
  {
    int write_result = pwrite(fd, (u8 *)data + total_num_bytes_written, total_num_bytes_left_to_write, begin + total_num_bytes_written);
    if(write_result >= 0)
    {
      total_num_bytes_written += write_result;
      total_num_bytes_left_to_write -= write_result;
    }
    else if(errno != EINTR)
    {
      break;
    }
  }
  return total_num_bytes_written;
}

internal OS_FileProps
os_properties_from_file(OS_Handle file)
{
  if(file.u64 == 0) { return (OS_FileProps){0}; }
  int fd = (int)file.u64;
  struct stat fd_stat = {0};
  int fstat_result = fstat(fd, &fd_stat);
  OS_FileProps props = {0};
  if(fstat_result != -1)
  {
    props = os_linx_file_props_from_stats(&fd_stat);
  }
  return props;
}

internal OS_Time_Stamp
os_time_now()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (OS_Time_Stamp)ts.tv_sec * os_time_frequency() + (OS_Time_Stamp)ts.tv_nsec;
}

internal OS_Time_Stamp
os_time_frequency()
{
	return 1000000000ULL;
}

internal void
os_sleep_ms(u32 ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

internal OS_FileProps 
os_linx_file_props_from_stats(struct stat *s)
{
	OS_FileProps props = {0};

	props.size     = (usize)s->st_size;
	props.created  = os_linx_time_from_timespec(s->st_ctim);
	props.modified = os_linx_time_from_timespec(s->st_mtim);

	if (S_ISDIR(s->st_mode)) {
		props.flags |= FileProp_IsFolder;
	}

	return props;
}


internal u64
os_linx_time_from_timespec(struct timespec in)
{
	return (u64)in.tv_sec * 1000000000ull + (u64)in.tv_nsec;
}


