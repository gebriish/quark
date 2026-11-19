#ifndef BASE_FILES_H
#define BASE_FILES_H

typedef struct FileId FileId;
struct FileId
{
  U64 u64[2];
};

typedef U32 FileFlags;
enum
{
  FileFlag_Readable  = (1u << 0),
  FileFlag_Writeable = (1u << 1),
};

typedef U32 FileKind;
enum
{
  FileKind_DNE,
  FileKind_Regular,
  FileKind_Directory,
  FileKind_Undetermined,
  FileKind_Symlink,
  FileKind_COUNT,
};

typedef struct FileProperty FileProperty;
struct FileProperty
{
  FileId id;
  FileKind kind;
  FileFlags flags;
  Time created;
  Time modified;
  S64 size;
};

///////////////////////////////
// @section: handle helpers

function B32 file_id_zero(FileId id);
function B32 file_id_matches(FileId a, FileId b);

///////////////////////////////
// @section: file loading and handling

function FileProperty file_property(String8 path);
function S64     load_file_to_buffer(String8 path, U8 *raw, S64 cap);
function String8 load_entire_file(Arena *arena, String8 path);
function B32     write_file_to_path(String8 data, String8 path);
function B32     create_directory(String8 path);

#endif // BASE_FILES_H
