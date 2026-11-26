
function b32
window_zero(Win_Handle w)
{
  return w.v[0] == 0;
}

function b32
window_matches(Win_Handle a, Win_Handle b)
{
  return a.v[0] == b.v[0];
}

function Win_Event *
window_event_push(Arena *arena, Win_Event_List *list, Win_Event_Kind kind)
{
  Win_Event *e = arena_push(arena, Win_Event, 1);
  e->kind = kind;
  SLLQueuePush(list->first, list->last, e);
  list->len += 1;
  return e;
}

function Win_Region
window_region_from_pixel(Win_Handle win, Vec2F32 pos, b32 *in_client)
{
  Vec4F32 region = window_get_move_region(win);
  const f32 resize_edge = 8.0f;

  Vec2F32 size = window_get_size(win);

  b32 left   = pos.x <  resize_edge;
  b32 right  = pos.x >= size.x - resize_edge;
  b32 top    = pos.y <  resize_edge;
  b32 bottom = pos.y >= size.y - resize_edge;

  Win_Region code = Win_Region_Move;
  b32 client = 0;

       if(top && left)     { code = Win_Region_TopLeft; }
  else if(top && right)    { code = Win_Region_TopRight; }
  else if(bottom && left)  { code = Win_Region_BottomLeft; }
  else if(bottom && right) { code = Win_Region_BottomRight; }
  else if(top)             { code = Win_Region_Top; }
  else if(bottom)          { code = Win_Region_Bottom; }
  else if(left)            { code = Win_Region_Left; }
  else if(right)           { code = Win_Region_Right; }
  else if(pos.y <  region.xyzw[Edge_Top] ||
          pos.x >= size.x - region.xyzw[Edge_Right]  ||
          pos.y >= size.y - region.xyzw[Edge_Bottom] ||
          pos.x <  region.xyzw[Edge_Left])
  {
    code = Win_Region_Move;
  }
  else
  {
    client = 1;
  }

  if (in_client) 
  {
    *in_client = client;
  }
  return code;
}

function Win_Cursor
window_region_to_cursor(Win_Region code)
{
  Win_Cursor cursor = Win_Cursor_Pointer;
  switch (code)
  {
    case Win_Region_TopLeft     : cursor = Win_Cursor_UpLeft; break;
    case Win_Region_Top         : cursor = Win_Cursor_UpDown; break;
    case Win_Region_TopRight    : cursor = Win_Cursor_UpRight; break;
    case Win_Region_Right       : cursor = Win_Cursor_LeftRight; break;
    case Win_Region_BottomRight : cursor = Win_Cursor_DownRight; break;
    case Win_Region_Bottom      : cursor = Win_Cursor_UpDown; break;
    case Win_Region_BottomLeft  : cursor = Win_Cursor_DownLeft; break;
    case Win_Region_Left        : cursor = Win_Cursor_LeftRight; break;
    case Win_Region_Move        : cursor = Win_Cursor_Pointer; break;
  }
  return cursor;
}
