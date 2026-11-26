
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

function String8
parent_relative_diretory_from_absolute(String8 parent, String8 dir) 
{
  String8 result = {0};
  if (str8_starts_with(dir, parent))
  {
    result = str8_skip(dir, parent.len + 1); 
  }
  else
  {
    String8 home = get_home_directory();
    if (str8_starts_with(dir, home))
    {
      result = str8_skip(dir, home.len + 1);
    }
    else
    {
      result = dir;
    }
  }
  return result;
}
