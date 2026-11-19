
function b32
gfx_window_zero(GFX_Window w)
{
  return w.v[0] == 0;
}

function b32
gfx_window_matches(GFX_Window a, GFX_Window b)
{
  return a.v[0] == b.v[0];
}

function GFX_Event *
gfx_event_list_push(Arena *arena, GFX_EventList *list, GFX_EventKind kind)
{
  GFX_Event *e = arena_push(arena, GFX_Event, 1);
  e->kind = kind;
  SLLQueuePush(list->first, list->last, e);
  list->len += 1;
  return e;
}

function GFX_MoveResize
gfx_window_moveresize_code(GFX_Window win, Vec2F32 pos, b32 *in_client)
{
  Vec4F32 region = gfx_window_move_region(win);
  const f32 resize_edge = 8.0f;

  Vec2F32 size = gfx_window_size(win);

  b32 left   = pos.x <  resize_edge;
  b32 right  = pos.x >= size.x - resize_edge;
  b32 top    = pos.y <  resize_edge;
  b32 bottom = pos.y >= size.y - resize_edge;

  GFX_MoveResize code = GFX_MoveResize_Move;
  b32 client = 0;

       if(top && left)     { code = GFX_MoveResize_TopLeft; }
  else if(top && right)    { code = GFX_MoveResize_TopRight; }
  else if(bottom && left)  { code = GFX_MoveResize_BottomLeft; }
  else if(bottom && right) { code = GFX_MoveResize_BottomRight; }
  else if(top)             { code = GFX_MoveResize_Top; }
  else if(bottom)          { code = GFX_MoveResize_Bottom; }
  else if(left)            { code = GFX_MoveResize_Left; }
  else if(right)           { code = GFX_MoveResize_Right; }
  else if(pos.y <  region.xyzw[Edge_Top] ||
          pos.x >= size.x - region.xyzw[Edge_Right]  ||
          pos.y >= size.y - region.xyzw[Edge_Bottom] ||
          pos.x <  region.xyzw[Edge_Left])
  {
    code = GFX_MoveResize_Move;
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

function GFX_Cursor
gfx_cursor_from_moveresize(GFX_MoveResize code)
{
  GFX_Cursor cursor = GFX_Cursor_Pointer;
  switch (code)
  {
    case GFX_MoveResize_TopLeft     : cursor = GFX_Cursor_UpLeft; break;
    case GFX_MoveResize_Top         : cursor = GFX_Cursor_UpDown; break;
    case GFX_MoveResize_TopRight    : cursor = GFX_Cursor_UpRight; break;
    case GFX_MoveResize_Right       : cursor = GFX_Cursor_LeftRight; break;
    case GFX_MoveResize_BottomRight : cursor = GFX_Cursor_DownRight; break;
    case GFX_MoveResize_Bottom      : cursor = GFX_Cursor_UpDown; break;
    case GFX_MoveResize_BottomLeft  : cursor = GFX_Cursor_DownLeft; break;
    case GFX_MoveResize_Left        : cursor = GFX_Cursor_LeftRight; break;
    case GFX_MoveResize_Move        : cursor = GFX_Cursor_Pointer; break;
  }
  return cursor;
}
