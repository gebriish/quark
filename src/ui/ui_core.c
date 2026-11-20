function b32
ui_key_zero(UI_Key key)
{
  return (key.v[0] == 0);
}

function b32
ui_key_matches(UI_Key a, UI_Key b)
{
  return (a.v[0] == b.v[0]);
}

function void
ui_init(void)
{
  Arena *arena = arena_reserve(Megabytes(4));
  ui_state = arena_push(arena, UI_State, 1);
  ui_state->arena = arena;
  ui_state->build_arena = arena_reserve(Megabytes(2));
}

function void
ui_begin_layout(UI_FrameInput input)
{
  arena_free(ui_state->build_arena, false);
  ui_state->frame_index += 1;
  ui_state->root = arena_push(ui_state->build_arena, UI_Box, 1);
  ui_state->call_list = ui_state->root;
  ui_state->root->final_dim = input.resolution;
  ui_state->last_input = ui_state->curr_input;
  ui_state->curr_input = input;
}

function void
ui_end_layout(R_DrawList *draw_list)
{
  UI_Box *root_node = ui_state->root;

  if (ui_state->curr_input.mouse_down == false) 
  {
    ui_state->hot.v[0] = 0;
  }
  if (ui_state->curr_input.mouse_down && !ui_state->last_input.mouse_down) 
  {
    ui_state->active.v[0] = 0;
  }

  // due to referencing the parent, recursion should
  // start from root only for grow layout pass.
  ui_grow_layout_pass(root_node);

  // handle hot and active nodes
  {
    UI_Key hot = ui_state->hot;

    if (!ui_key_zero(hot))
    {
      UI_Box *box = ui_get_persist_node(hot)->box;
    }
  }

  for EachNode(node, UI_Box, root_node->first)
  {
    ui_position_layout_pass(node);
    ui_draw_layout_pass(node, draw_list);
  }

  // run persist gc
  for EachIndex(i, CArrayLength(ui_state->persist_buckets))
  {
    UI_Persist *next = 0;
    for (UI_Persist *p = ui_state->persist_buckets[i]; p; p = next)
    {
      next = p->hash_next;
      if (p->last_touched_frame + 2 < ui_state->frame_index)
      {
        if (p->hash_prev) p->hash_prev->hash_next = p->hash_next;
        else              ui_state->persist_buckets[i] = p->hash_next;
        if (p->hash_next) p->hash_next->hash_prev = p->hash_prev;

        p->next = ui_state->persist_free;
        ui_state->persist_free = p;
      }
      else
      {
        p->spawn_t += ui_state->curr_input.delta_time;
      }
    }
  }
}

function UI_Box *
ui_open(UI_Box *parent)
{
  UI_Box *box = arena_push(ui_state->build_arena, UI_Box, 1);
  box->parent = parent;
  DLLPushBack(parent->first, parent->last, box);
  SLLStackPush_N(ui_state->call_list, box, call_prev);
  parent->child_count += 1;
  return box;
}

function void
ui_box_configure(UI_Config *config)
{
  UI_Box *box = ui_state->call_list;

#define X(type, name) box->name = config->name;
  X_BOX_CONFIG_FIELDS
#undef X

  // Fixed sized axes can be finalized here itself.

  if (box->size.width.kind == UI_SizeKind_Fixed)
    box->final_dim.x = box->size.width.value;

  if (box->size.height.kind == UI_SizeKind_Fixed)
    box->final_dim.y = box->size.height.value;

  // Persistent Node

  if (box->label.len)
  {
    UI_Key key = ui_key_from_string(ui_state->persist_stack ? ui_state->persist_stack->key : (UI_Key) {0}, box->label);
    UI_Persist *persist = ui_get_persist_node(key);
    persist->box = box;
    box->persist = persist;
    SLLStackPush(ui_state->persist_stack, persist);
    if (box->flags & UI_BoxFlag_AutoActive)
    {
      ui_state->active = key;
    }
  }

}

function void
ui_close(void)
{
  UI_Box *box = ui_state->call_list;

  Axis2D layout_axis = box->layout;
  Axis2D layout_perp = axis2d_flip(layout_axis);

  Vec2F32 content_dim = {0};

  // By now , all the children with UI_SizeKind_Fixed, UI_SizeKind_Fit are
  // resolved, so we can use those to compute [box]'s UI_SizeKind_Fit sizing.

  for EachNode(child, UI_Box, box->first)
  {
    content_dim.xy[layout_axis] += child->final_dim.xy[layout_axis];
    content_dim.xy[layout_perp] = Max(content_dim.xy[layout_perp], child->final_dim.xy[layout_perp]);
  }

  f32 gap_size = (f32) Max(box->child_count - (s64) 1, 0) * box->gap;

  content_dim.xy[layout_axis] += gap_size;

  box->content_dim = content_dim;

  Vec2F32 padding_sum =
  {
    box->padding.values[Edge_Left] + box->padding.values[Edge_Right],
    box->padding.values[Edge_Top] + box->padding.values[Edge_Bottom],
  };

  if (box->size.width.kind == UI_SizeKind_Fit) box->final_dim.x = content_dim.x + padding_sum.x;
  if (box->size.height.kind == UI_SizeKind_Fit) box->final_dim.y = content_dim.y + padding_sum.y;

  SLLStackPop_N(ui_state->call_list, call_prev);
  if (box->persist)
  {
    SLLStackPop(ui_state->persist_stack);
  }
}

function void
ui_grow_layout_pass(UI_Box *box)
{
  f32 k = 1.f - expf(-40.f * ui_state->curr_input.delta_time);

  if (box->persist)
  {
    box->persist->last_blur = Lerp(box->persist->last_blur, box->blur_radius, 1.f);
  }

  Axis2D layout_axis = box->layout;
  Axis2D layout_perp = axis2d_flip(layout_axis);

  u64 layout_flag = (layout_axis == Axis2D_Horizontal) ? UI_BoxFlag_SmoothWidth  : UI_BoxFlag_SmoothHeight;
  u64 perp_flag   = (layout_axis == Axis2D_Horizontal) ? UI_BoxFlag_SmoothHeight : UI_BoxFlag_SmoothWidth;

  Vec2F32 padding_sum =
  {
    box->padding.values[Edge_Left] + box->padding.values[Edge_Right],
    box->padding.values[Edge_Top] + box->padding.values[Edge_Bottom],
  };

  Vec2F32 available = vec2_sub(box->final_dim, padding_sum);
  f32 gap_size = (f32) Max(box->child_count - (s64) 1, 0) * box->gap;

  // pass 1: resolve percent sizes, smooth non-grow axes, measure occupied space
  f32 occupied = gap_size;
  f32 grow_weight = 0.f;
  for EachNode(child, UI_Box, box->first)
  {
    UI_Persist *p = child->persist;
    UI_SizeAxis *w_size = &child->size.width;
    UI_SizeAxis *h_size = &child->size.height;

    if (w_size->kind == UI_SizeKind_Percent) child->final_dim.x = available.x * w_size->value;
    if (h_size->kind == UI_SizeKind_Percent) child->final_dim.y = available.y * h_size->value;

    if (p && w_size->kind != UI_SizeKind_Grow)
    {
      f32 s = (child->flags & UI_BoxFlag_SmoothWidth) ? k : 1.f;
      p->last_dim.x += (child->final_dim.x - p->last_dim.x) * s;
      child->final_dim.x = p->last_dim.x;
    }

    if (p && h_size->kind != UI_SizeKind_Grow)
    {
      f32 s = (child->flags & UI_BoxFlag_SmoothHeight) ? k : 1.f;
      p->last_dim.y += (child->final_dim.y - p->last_dim.y) * s;
      child->final_dim.y = p->last_dim.y;
    }

    UI_SizeAxis size = child->size.axes[layout_axis];
    if (size.kind == UI_SizeKind_Grow)
    {
      grow_weight += size.value;
    }
    else
    {
      occupied += child->final_dim.xy[layout_axis];
    }
  }

  f32 remaining = Max(available.xy[layout_axis] - occupied, 0.f);

  // pass 2: resolve grow axes, smooth them, accumulate smoothed content size
  Vec2F32 content_dim = {0};
  for EachNode(child, UI_Box, box->first)
  {
    UI_Persist *p = child->persist;
    UI_SizeAxis size = child->size.axes[layout_axis];

    if (size.kind == UI_SizeKind_Grow)
    {
      child->final_dim.xy[layout_axis] = (grow_weight > 0.f) ? remaining * size.value / grow_weight : 0.f;

      if (p)
      {
        f32 s = (child->flags & layout_flag) ? k : 1.f;
        p->last_dim.xy[layout_axis] += (child->final_dim.xy[layout_axis] - p->last_dim.xy[layout_axis]) * s;
        child->final_dim.xy[layout_axis] = p->last_dim.xy[layout_axis];
      }
    }

    if (child->size.axes[layout_perp].kind == UI_SizeKind_Grow)
    {
      child->final_dim.xy[layout_perp] = available.xy[layout_perp];

      if (p)
      {
        f32 s = (child->flags & perp_flag) ? k : 1.f;
        p->last_dim.xy[layout_perp] += (child->final_dim.xy[layout_perp] - p->last_dim.xy[layout_perp]) * s;
        child->final_dim.xy[layout_perp] = p->last_dim.xy[layout_perp];
      }
    }

    content_dim.xy[layout_axis] += child->final_dim.xy[layout_axis];
    content_dim.xy[layout_perp] = Max(content_dim.xy[layout_perp], child->final_dim.xy[layout_perp]);
  }
  content_dim.xy[layout_axis] += gap_size;
  box->content_dim = content_dim;

  // recurse: children's final_dim is already smoothed
  for EachNode(child, UI_Box, box->first)
  {
    ui_grow_layout_pass(child);
  }
}

function void
ui_position_layout_pass(UI_Box *box)
{
  UI_Persist *p = box->persist;
  if (p)
  {
    f32 k = 1.f - expf(-30.f * ui_state->curr_input.delta_time);
    Vec2F32 scale = v2f32(1.f, 1.f);

    if (box->flags & UI_BoxFlag_SmoothX) scale.x = k;
    if (box->flags & UI_BoxFlag_SmoothY) scale.y = k;

    p->last_pos = vec2_add(p->last_pos, vec2_mul(vec2_sub(box->final_pos, p->last_pos), scale));
    box->final_pos = p->last_pos;
  }

  Axis2D layout_axis = box->layout;
  Axis2D layout_perp = axis2d_flip(box->layout);

  Vec2F32 inner_min =
  {
    box->final_pos.x + box->padding.values[Edge_Left],
    box->final_pos.y + box->padding.values[Edge_Top],
  };

  Vec2F32 inner_dim =
  {
    box->final_dim.x - box->padding.values[Edge_Left] - box->padding.values[Edge_Right],
    box->final_dim.y - box->padding.values[Edge_Top]  - box->padding.values[Edge_Bottom],
  };

  f32 leftover_main = inner_dim.xy[layout_axis] - box->content_dim.xy[layout_axis];
  Vec2F32 offset = pivot_offsets[box->pivot];

  Vec2F32 cursor = inner_min;
  cursor.xy[layout_axis] += leftover_main * offset.xy[layout_axis];

  Rng2F32 box_rect =
  {
    .min = box->final_pos,
    .max = vec2_add(box->final_pos, box->final_dim)
  };

  if (box->persist && Rng2Check(ui_state->curr_input.mouse_pos.xy, box_rect))
  {
    UI_Key key = box->persist->key;

    if (ui_state->curr_input.mouse_down == false) 
    {
      ui_state->hot = key;
    }
    else if (ui_state->last_input.mouse_down == false)
    {
      ui_state->active = key;
    }
  }

  for EachNode(child, UI_Box, box->first)
  {
    Vec2F32 pos = cursor; // @todo: scroll offset
    pos.xy[layout_perp] += (inner_dim.xy[layout_perp] - child->final_dim.xy[layout_perp]) * offset.xy[layout_perp];

    child->final_pos = pos;
    ui_position_layout_pass(child);

    cursor.xy[layout_axis] += child->final_dim.xy[layout_axis] + box->gap;
  }
}

function void
ui_draw_layout_pass(UI_Box *box, R_DrawList *draw_list)
{
  // precompute stuff.
  Rng2F32 box_rect =
  {
    .min = box->final_pos,
    .max = vec2_add(box->final_pos, box->final_dim)
  };

  Rng2F32 inner_rect =
  {
    .min = vec2_add(box_rect.min, v2f32(box->padding.values[Edge_Left], box->padding.values[Edge_Top])),
    .max = vec2_sub(box_rect.max, v2f32(box->padding.values[Edge_Right], box->padding.values[Edge_Bottom])),
  };

  box_rect.min.x = floorf(box_rect.min.x);
  box_rect.min.y = floorf(box_rect.min.y);
  box_rect.max.x = floorf(box_rect.max.x);
  box_rect.max.y = floorf(box_rect.max.y);

  inner_rect.min.x = floorf(inner_rect.min.x);
  inner_rect.min.y = floorf(inner_rect.min.y);
  inner_rect.max.x = ceilf(inner_rect.max.x);
  inner_rect.max.y = ceilf(inner_rect.max.y);

  // draw fill rect
  if (box->fill_color.a >= 1e-5f)
  {
    if (box->blur_radius >= 2.f)
    {
      r_push_blur(draw_list,
                  .blur = box->blur_radius,
                  .rect = box_rect,
                  .tint = box->fill_color,
                  .radii = box->radii);
    }
    else
    {
      r_push_quad(draw_list, 
                  box->texture,
                  .dst = box_rect,
                  .src = { .min = {0,0}, .max = {1,1} },
                  .col = box->fill_color,
                  .radii = box->radii);
    }
  }

  // draw border rect
  if (box->border_color.a >= 1e-5f)
  {
    r_push_quad(draw_list, r_empty_handle(),
        .dst = box_rect,
        .col = box->border_color,
        .thickness = Max(1.f, box->border_thickness),
        .radii = box->radii);
  }

  if (box->flags & UI_BoxFlag_Clip)
  {
    r_push_clip(draw_list, inner_rect);
  }

  // draw text
  if (box->text_color.a >= 1e-5f && box->text.len > 0 &&
      !r_fnt_zero(box->font) && box->text_size > 0)
  {
    r_push_text_simple(draw_list,
        box->font,
        box->text,
        inner_rect.min,
        box->text_size,
        2,
        box->text_bold,
        box->text_color);;
  }

  // recurse
  for EachNode(child, UI_Box, box->first)
  {
    if (child->shadow_color.a > 0.05f)
    {
      const f32 shadow_blur = 50.f;
      const f32 shadow_offset = 30.f;
      Rng2F32 shadow_rect =
      {
        .min = vec2_sub(child->final_pos, v2f32(shadow_offset,shadow_offset)),
        .max = vec2_add(vec2_add(child->final_pos, child->final_dim), v2f32(shadow_offset,shadow_offset)),
      };

      r_push_quad(draw_list, r_empty_handle(),
                  .dst = shadow_rect,
                  .edge_blur = shadow_blur,
                  .src = { .min = {0,0}, .max = {1,1} },
                  .col = child->shadow_color,
                  .radii = v4f32(shadow_blur, shadow_blur, shadow_blur, shadow_blur));
    }

    ui_draw_layout_pass(child, draw_list);
  }

  if (box->flags & UI_BoxFlag_Clip)
  {
    r_pop_clip(draw_list);
  }
}

function UI_SizeAxis
ui_size_fixed(f32 x)
{
  UI_SizeAxis size = {
    .kind = UI_SizeKind_Fixed,
    .value = x
  };
  return size;
}

function UI_SizeAxis
ui_size_percent(f32 x)
{
  UI_SizeAxis size = {
    .kind = UI_SizeKind_Percent,
    .value = x
  };
  return size;
}

function UI_SizeAxis
ui_size_grow(f32 x)
{
  UI_SizeAxis size = {
    .kind = UI_SizeKind_Grow,
    .value = x
  };
  return size;
}

function UI_SizeAxis
ui_size_fit(void)
{
  UI_SizeAxis size = {
    .kind = UI_SizeKind_Fit,
    .value = 0.f
  };
  return size;
}

function UI_Padding
ui_padding(u16 v)
{
  UI_Padding pad = { v, v, v, v };
  return pad;
}

function UI_Padding
ui_padding_xy(u16 x, u16 y)
{
  UI_Padding pad;
  pad.values[Edge_Left]   = x;
  pad.values[Edge_Right]  = x;
  pad.values[Edge_Top]    = y;
  pad.values[Edge_Bottom] = y;
  return pad;
}

function UI_Key
ui_key_from_string(UI_Key seed, String8 string)
{
  u64 h = 14695981039346656037ull ^ seed.v[0];
  for (u64 i = 0; i < string.len; i += 1)
  {
    h ^= string.raw[i];
    h *= 1099511628211ull;
  }
  UI_Key key = { .v = { h } };
  return key;
}

function UI_Persist *
ui_get_persist_node(UI_Key key)
{
  u64 bucket_index = key.v[0] % CArrayLength(ui_state->persist_buckets);
  UI_Persist *persist =0;
  for (UI_Persist *p = ui_state->persist_buckets[bucket_index]; p; p = p->hash_next)
  {
    if (ui_key_matches(p->key, key))
    {
      persist = p;
      break;
    }
  }

  if (persist == 0)
  {
    persist = ui_state->persist_free;
    if (ui_state->persist_free)
    {
      SLLStackPop(ui_state->persist_free);
      MemZeroStruct(persist);
    }
    else
    {
      persist = arena_push(ui_state->arena, UI_Persist, 1);
    }

    persist->key = key;
    UI_Persist *head = ui_state->persist_buckets[bucket_index];
    persist->hash_next = head;
    if (head) head->hash_prev = persist;
    ui_state->persist_buckets[bucket_index] = persist;
    persist->spawn_t = 0;
  }

  persist->last_touched_frame = ui_state->frame_index;
  return persist;
}


function UI_Box *
ui_get_current_box(void)
{
  return ui_state->call_list;
}

function UI_Signal
ui_get_signal_from_box(UI_Box *box)
{
  UI_Signal sig = {0};

  if (box->persist)
  {
    sig.hovered  = (u8) ui_key_matches(ui_state->hot, box->persist->key);
    sig.active   = (u8) ui_key_matches(ui_state->active, box->persist->key);
    sig.pressed  = sig.active && sig.hovered && ui_state->curr_input.mouse_down;
    sig.clicked  = sig.hovered && (ui_state->curr_input.mouse_down && !ui_state->last_input.mouse_down);
    sig.released = sig.hovered && (!ui_state->curr_input.mouse_down && ui_state->last_input.mouse_down);
  }

  return sig;
}
