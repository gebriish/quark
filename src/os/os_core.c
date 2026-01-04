#include "os_core.h"

internal String8
os_data_from_file_path(Arena *arena, String8 path)
{
	OS_Handle file = os_file_open(OS_AccessFlag_Read | OS_AccessFlag_ShareRead, path, arena);
	OS_FileProps props = os_properties_from_file(file);
	String8 data = os_string_from_file_range(arena, file, 0, props.size);
	os_file_close(file);
	return data;
}

internal bool
os_write_data_to_file_path(String8 path, String8 data, Arena *scratch)
{
  bool good = false;
  OS_Handle file = os_file_open(OS_AccessFlag_Write, path, scratch);
  if(file.u64 != 0)
  {
    usize bytes_written = os_file_write(file, 0, data.len, data.str);
    good = (bytes_written == data.len);
    os_file_close(file);
  }
  return good;
}

internal String8
os_string_from_file_range(Arena *arena, OS_Handle file, usize begin, usize end)
{
  usize pre_pos = arena->pos;
  String8 result;
  result.len = end - begin;
  result.str = arena_push_array(arena, u8, result.len);
  usize actual_read_size = os_file_read(file, begin, end, result.str);
  if(actual_read_size < result.len)
  {
    arena_pop_to(arena, pre_pos + actual_read_size);
    result.len = actual_read_size;
  }
  return result;
}

internal OS_Time_Duration
os_time_diff(OS_Time_Stamp start, OS_Time_Stamp end)
{
	OS_Time_Duration result;
	u64 freq = os_time_frequency();
	u64 elapsed = end - start;

	result.seconds = (f64)elapsed / (f64)freq;
	result.milliseconds = result.seconds * 1000.0;
	result.microseconds = result.seconds * 1000000.0;

	return result;
}
