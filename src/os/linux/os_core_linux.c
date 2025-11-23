#include "../os_core.h"

#include <sys/stat.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>
#include <time.h>

//////////////////////////////////////////

internal usize
os_file_size(String8 path)
{
	struct stat st;
	if (stat((char *)path.raw, &st) == -1) {
		return 0;
	}
	return (usize)st.st_size;
}

internal usize
os_read_file_into_buffer(u8 *buffer, usize capacity, String8 path)
{
	int fd = open((char *)path.raw, O_RDONLY);
	if (fd == -1) return 0;
	
	struct stat st;
	if (fstat(fd, &st) == -1) {
		close(fd);
		return 0;
	}
	
	usize file_size = (usize)st.st_size;
	usize bytes_to_read = file_size < capacity ? file_size : capacity;
	
	ssize_t bytes_read = read(fd, buffer, bytes_to_read);
	close(fd);
	
	if (bytes_read < 0) return 0;
	return (usize)bytes_read;
}

internal String8
os_read_file_data(Allocator *allocator, String8 path)
{
	String8 result = {0};
	
	usize file_size = os_file_size(path);
	if (file_size == 0) return result;
	
	result.raw = mem_alloc(allocator, sizeof(u8) * file_size, AlignOf(u8)).mem;
	result.len = os_read_file_into_buffer(result.raw, file_size, path);
	
	if (result.len == 0) {
		result.raw = 0;
	}
	
	return result;
}

internal bool
os_write_file_data(String8 path, String8 data)
{
	int fd = open((char *)path.raw, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) return false;

	ssize_t bytes_written = write(fd, data.raw, data.len);
	close(fd);

	return bytes_written == (ssize_t)data.len;
}


//////////////////////////////////////////

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
