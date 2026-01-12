#ifndef OS_CORE_H
#define OS_CORE_H

#include "../base/base_core.h"
#include "../base/base_string.h"

typedef struct { u64 u64; } OS_Handle;

Enum(OS_Access_Flags, u32) {
	OS_AccessFlag_Read       = (1 << 0),
	OS_AccessFlag_Write      = (1 << 1),
	OS_AccessFlag_Execute    = (1 << 2),
	OS_AccessFlag_Append     = (1 << 3),
	OS_AccessFlag_ShareRead  = (1 << 4),
	OS_AccessFlag_ShareWrite = (1 << 5),
	OS_AccessFlag_Inherited  = (1 << 6),
};

Enum(File_Prop_Flags, u32) {
	FileProp_IsFolder = (1 << 0)
};

typedef struct OS_FileProps OS_FileProps;
struct OS_FileProps {
	usize size;
	u64 modified;
	u64 created;
	File_Prop_Flags flags;
};

internal void *os_reserve(usize size);
internal int  os_commit(void *ptr, usize size);
internal void  os_decommit(void *ptr, usize size);
internal void  os_release(void *ptr, usize size);

internal OS_Handle     os_file_open(OS_Access_Flags flags, String8 path, Arena *scratch);
internal void          os_file_close(OS_Handle file);
internal usize         os_file_read(OS_Handle handle, usize begin, usize end, void *out_data);
internal usize         os_file_write(OS_Handle file, usize begin, usize end, u8 *data);
internal OS_FileProps  os_properties_from_file(OS_Handle file);

internal String8 os_data_from_file_path(Arena *arena, String8 path);
internal String8 os_string_from_file_range(Arena *arena, OS_Handle file, usize begin, usize end);
internal bool    os_write_data_to_file_path(String8 path, String8 data, Arena *scratch);

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
