function R_Handle 
r_empty_handle(void)
{
  R_Handle h = {0};
  return h;
}

function B32 
r_handle_zero(R_Handle h)
{
  return (h.u64[0] == 0);
}

function B32 
r_handle_matches(R_Handle a, R_Handle b)
{
  return (a.u64[0] == b.u64[0]);
}

function void 
r_push_quad_(R_DrawList *list, R_Handle tex, R_Quad *quad)
{
  Arena *arena = r_frame_arena();

  R_Draw *draw = list->last;

  B32 can_batch = draw && draw->kind == R_DrawKind_Quads &&
                  r_handle_matches(draw->quad_batch.tex, tex) &&
                  draw->quad_batch.len < QUAD_BATCH_SIZE;

  if (can_batch == false) 
  {
    draw = arena_push(arena, R_Draw, 1);
    draw->kind = R_DrawKind_Quads;
    draw->quad_batch.raw = arena_push_non_zero(arena, R_Quad, QUAD_BATCH_SIZE);
    draw->quad_batch.tex = tex;
    SLLQueuePush(list->first, list->last, draw);
    list->len += 1;
  }

  draw->quad_batch.raw[draw->quad_batch.len] = *quad;
  draw->quad_batch.len += 1;
}

function void 
r_push_blur_(R_DrawList *list, R_Blur *blur)
{
  Arena *arena = r_frame_arena();
  R_Draw *draw = arena_push(arena, R_Draw, 1);
  draw->kind = R_DrawKind_Blur;
  draw->blur = *blur;

  SLLQueuePush(list->first, list->last, draw);
  list->len += 1;
}

function void
r_push_text_quad_(R_DrawList *list, R_Handle tex, R_TextQuad *q)
{
  R_Draw *draw = list->last;
  B32 can_batch = draw && draw->kind == R_DrawKind_Text &&
                  r_handle_matches(draw->text_batch.tex, tex) &&
                  draw->text_batch.len < QUAD_BATCH_SIZE;
  if(!can_batch)
  {
    draw = arena_push(r_frame_arena(), R_Draw, 1);
    draw->kind = R_DrawKind_Text;
    draw->text_batch.raw = arena_push_non_zero(r_frame_arena(), R_TextQuad, QUAD_BATCH_SIZE);
    draw->text_batch.tex = tex;
    SLLQueuePush(list->first, list->last, draw);
    list->len += 1;
  }
  draw->text_batch.raw[draw->text_batch.len] = *q;
  draw->text_batch.len += 1;
}

function void 
r_push_clip(R_DrawList *list, Rng2F32 rect)
{
  Arena *arena = r_frame_arena();
  R_Draw *draw = arena_push(arena, R_Draw, 1);
  draw->kind = R_DrawKind_ClipPush;

  draw->rect.min.x = (S32) rect.min.x;
  draw->rect.min.y = (S32) rect.min.y;
  draw->rect.max.x = (S32) rect.max.x;
  draw->rect.max.y = (S32) rect.max.y;

  SLLQueuePush(list->first, list->last, draw);
  list->len += 1;
}

function void 
r_pop_clip(R_DrawList *list)
{
  Arena *arena = r_frame_arena();
  R_Draw *draw = arena_push(arena, R_Draw, 1);
  draw->kind = R_DrawKind_ClipPop;
  SLLQueuePush(list->first, list->last, draw);
  list->len += 1;
}

function R_Image
r_blank_image(R_TexFormat fmt, Vec2S32 dim)
{
  R_Image image = {
    .data = 0,
    .dim = dim,
    .fmt = fmt
  };
  return image;
}

function F32
r_frame_begin(void)
{
  static Time prev = 0;
  Time curr = time_now();
  F32 delta_time = (F32) time_diff(prev, curr).seconds;
  prev = curr;
  return Min(delta_time, 1.f/30.f);
}


