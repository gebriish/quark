
function B32 
file_id_zero(FileId id)
{
  return (id.u64[0] == 0) && (id.u64[1] == 0);
}

function B32 
file_id_matches(FileId a, FileId b)
{
  return a.u64[0] == b.u64[0] &&
         a.u64[1] == b.u64[1];
}

function String8 
load_entire_file(Arena *arena, String8 path)
{
  S64 size = load_file_to_buffer(path, 0, 0);
  U8 *raw = arena_push(arena, U8, size);
  load_file_to_buffer(path, raw, size);
  return str8(raw, size);
}
