#ifndef OS_CORE_H
#define OS_CORE_H

#include "../base/base_core.h"
#include "../base/base_string.h"

/////////////////////////////////////
// ~geb: File handling

internal usize   os_file_size(String8 path);
internal String8 os_read_file_data(Allocator *arena, String8 path);
internal bool    os_write_file_data(String8 path, String8 data);

internal usize   os_read_file_into_buffer(u8 *buffer, usize capacity, String8 path);


/////////////////////////////////////
// ~geb: Time API

typedef struct OS_Time_Duration OS_Time_Duration;
struct OS_Time_Duration {
	f64 seconds;
	f64 milliseconds;
	f64 microseconds;
};

typedef u64 OS_Time_Stamp;

internal OS_Time_Stamp os_time_now();
internal OS_Time_Stamp os_time_frequency();
internal void          os_sleep_ms(u32 ms);

internal OS_Time_Duration os_time_diff(OS_Time_Stamp start, OS_Time_Stamp end);

#endif
