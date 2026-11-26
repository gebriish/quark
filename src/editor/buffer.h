#ifndef BUFFER_H
#define BUFFER_H

typedef u32 Buffer_Flags;
enum
{
  Buffer_Flag_Write = 1 << 0,
  Buffer_Flag_Read  = 1 << 1,
};

typedef struct Buffer_Id Buffer_Id;
struct Buffer_Id
{
  u64 v[1];
};

typedef struct Buffer Buffer;
struct Buffer
{
  Arena *arena;
  String8 name;
  Buffer_Flags flags;
};

function Buffer *make_buffer(String8 name);
function void delete_buffer(Buffer *buffer);
function void write_buffer_to_path(Buffer *buffer, String8 path);

#endif // BUFFER_H
