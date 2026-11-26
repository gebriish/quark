#ifndef BASE_FILES_H
#define BASE_FILES_H

typedef struct File_Id File_Id;
struct File_Id
{
  u64 v[2];
};

typedef u32 File_Flags;
enum
{
  File_Flag_Readable  = (1u << 0),
  File_Flag_Writeable = (1u << 1),
};

typedef u32 File_Kind;
enum
{
  File_Kind_DNE,
  File_Kind_Regular,
  File_Kind_Directory,
  File_Kind_Undetermined,
  File_Kind_Symlink,
  File_Kind_COUNT,
};

typedef struct File_Info File_Info;
struct File_Info
{
  File_Id id;
  File_Kind kind;
  File_Flags flags;
  Time created;
  Time modified;
  s64 size;
};

///////////////////////////////
// @section: handle helpers

function b32 file_id_zero(File_Id id);
function b32 file_id_matches(File_Id a, File_Id b);

///////////////////////////////
// @section: file loading and handling

function File_Info file_get_info(String8 path);
function s64     load_file_to_buffer(String8 path, u8 *raw, s64 cap);
function String8 load_entire_file(Arena *arena, String8 path);
function b32     write_file_to_path(String8 data, String8 path);
function b32     create_directory(String8 path);
function String8 file_name_from_path(String8 path);

///////////////////////////////
// @section: paths

function b32 set_current_working_directory(String8 path);
function String8 get_current_working_directory(Arena *arena);
function String8 get_absolute_directory(Arena *arena, String8 rel);
function String8 get_home_directory(void);
function String8 parent_relative_diretory_from_absolute(String8 path, String8 dir);

#endif // BASE_FILES_H
