function b32 
rndr_handle_zero(Rndr_Handle h)
{
  return (h.v[0] == 0);
}

function b32 
rndr_handle_matches(Rndr_Handle a, Rndr_Handle b)
{
  return (a.v[0] == b.v[0]);
}

function void 
push_quad_(Draw_List *list, Rndr_Handle tex, Quad_Inst *quad)
{
  Arena *arena = frame_arena();

  Draw_Call *draw = list->last;

  b32 can_batch = draw && draw->kind == Draw_Kind_Quads &&
                  rndr_handle_matches(draw->quad_batch.tex, tex) &&
                  draw->quad_batch.len < QUAD_BATCH_SIZE;

  if (can_batch == false) 
  {
    draw = arena_push(arena, Draw_Call, 1);
    draw->kind = Draw_Kind_Quads;
    draw->quad_batch.raw = arena_push_non_zero(arena, Quad_Inst, QUAD_BATCH_SIZE);
    draw->quad_batch.tex = tex;
    SLLQueuePush(list->first, list->last, draw);
    list->len += 1;
  }

  draw->quad_batch.raw[draw->quad_batch.len] = *quad;
  draw->quad_batch.len += 1;
}

function void 
push_blur_(Draw_List *list, Quad_Blur *blur)
{
  Arena *arena = frame_arena();
  Draw_Call *draw = arena_push(arena, Draw_Call, 1);
  draw->kind = Draw_Kind_Blur;
  draw->blur = *blur;

  SLLQueuePush(list->first, list->last, draw);
  list->len += 1;
}

function void
push_text_quad_(Draw_List *list, Rndr_Handle tex, Quad_Text *q)
{
  Draw_Call *draw = list->last;
  b32 can_batch = draw && draw->kind == Draw_Kind_Text &&
                  rndr_handle_matches(draw->text_batch.tex, tex) &&
                  draw->text_batch.len < QUAD_BATCH_SIZE;
  if(!can_batch)
  {
    draw = arena_push(frame_arena(), Draw_Call, 1);
    draw->kind = Draw_Kind_Text;
    draw->text_batch.raw = arena_push_non_zero(frame_arena(), Quad_Text, QUAD_BATCH_SIZE);
    draw->text_batch.tex = tex;
    SLLQueuePush(list->first, list->last, draw);
    list->len += 1;
  }
  draw->text_batch.raw[draw->text_batch.len] = *q;
  draw->text_batch.len += 1;
}

function void 
push_clip(Draw_List *list, Rng2F32 rect)
{
  Arena *arena = frame_arena();
  Draw_Call *draw = arena_push(arena, Draw_Call, 1);
  draw->kind = Draw_Kind_ClipPush;

  draw->rect.min.x = (s32) rect.min.x;
  draw->rect.min.y = (s32) rect.min.y;
  draw->rect.max.x = (s32) rect.max.x;
  draw->rect.max.y = (s32) rect.max.y;

  SLLQueuePush(list->first, list->last, draw);
  list->len += 1;
}

function void 
pop_clip(Draw_List *list)
{
  Arena *arena = frame_arena();
  Draw_Call *draw = arena_push(arena, Draw_Call, 1);
  draw->kind = Draw_Kind_ClipPop;
  SLLQueuePush(list->first, list->last, draw);
  list->len += 1;
}

function f32
frame_begin(void)
{
  static Time prev = 0;
  Time curr = time_now();
  f32 delta_time = (f32) time_diff(prev, curr).seconds;
  prev = curr;
  return Min(delta_time, 1.f/30.f);
}


