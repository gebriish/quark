
function b32
file_id_zero(File_Id id)
{
  return (id.v[0] == 0) && (id.v[1] == 0);
}

function b32
file_id_matches(File_Id a, File_Id b)
{
  return a.v[0] == b.v[0] &&
         a.v[1] == b.v[1];
}

function String8
load_entire_file(Arena *arena, String8 path)
{
  s64 size = load_file_to_buffer(path, 0, 0);
  u8 *raw = arena_push(arena, u8, size);
  load_file_to_buffer(path, raw, size);
  return str8(raw, size);
}
