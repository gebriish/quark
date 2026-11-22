
const s64 R_FNT_ATLAS_DIM = 512;

function b32 
fnt_handle_zero(Fnt_Handle f)
{
  return (f.v[0] == 0);
}

function b32 
fnt_handle_matches(Fnt_Handle a, Fnt_Handle b)
{
  return (a.v[0] == b.v[0]);
}

//
// font glyph caching
//

function void
fnt_init(void)
{
  fnt_provider_init();

  Arena *arena = arena_reserve(Megabytes(4));
  fnt_state = arena_push(arena, Fnt_State, 1);
  fnt_state->arena = arena;
  Image blank = {
    .data = 0,
    .fmt = Pixel_Format_RGB8,
    .dim = v2s32(R_FNT_ATLAS_DIM, R_FNT_ATLAS_DIM)
  };
  fnt_state->texture = texture_alloc(blank);
}

function Glyph_Rect 
fnt_get_glyph(Fnt_Handle font, u32 size, b32 bold, u32 index)
{
  u128 key = {0};

  key.lo = font.v[0];
  key.hi = (u64) size << 32 | (u64) index | (u64) bold << 63;

  u64 hash = u128_hash(key);
  u64 hash_index = hash & (CArrayLength(fnt_state->slots) - 1);

  Glyph_Node *glyph_node = 0;
  Glyph_Hash_Bucket bucket = fnt_state->slots[hash_index];
  for each_node(node, Glyph_Node, bucket.first)
  {
    if (u128_matches(node->key, key)) 
    {
      glyph_node = node;
      break;
    }
  }

  if (!glyph_node)
  {
    glyph_node = fnt_state->free_glyphs;
    if (fnt_state->free_glyphs)
    {
      SLLStackPop(fnt_state->free_glyphs);
      MemZeroStruct(glyph_node);
    }
    else
    {
      glyph_node = arena_push(fnt_state->arena, Glyph_Node, 1);
    }

    Glyph_Rect glyph = {0};
    ScratchScope(scratch, 0, 0)
    {
      Font_Raster img = rasterize_glyph(font, size, index, bold, scratch.arena);

      if (fnt_state->cursor.x + img.dim.x >= R_FNT_ATLAS_DIM)
      {
        fnt_state->cursor.x = 0;
        fnt_state->cursor.y += fnt_state->row_height + 1;
      }

      Rng2S32 range = {
        .min = fnt_state->cursor,
        .max = { fnt_state->cursor.x + img.dim.x, fnt_state->cursor.y + img.dim.y },
      };

      texture_update(fnt_state->texture, range, img.data);
      fnt_state->cursor.x += img.dim.x + 1;
      fnt_state->row_height = Max(fnt_state->row_height, (s16) img.dim.y);

      glyph.offset = img.offset;
      glyph.advance = img.advance;
      glyph.rect = (Rng2S16) {
        .min = { (s16) range.min.x, (s16) range.min.y },
        .max = { (s16) range.max.x, (s16) range.max.y },
      };
    }

    glyph_node->glyph = glyph;
    glyph_node->key = key;
    DLLPushBack(fnt_state->slots[hash_index].first,
                fnt_state->slots[hash_index].last,
                glyph_node);
  }

  return glyph_node->glyph;
}

function void 
fnt_frame(void)
{
  
}

/////////////////////////////
// @section: pushing draw commands

function Vec2F32 
push_text_simple(Draw_List *list, Fnt_Handle font, String8 text, Vec2F32 pos, u32 size, s32 tab_width, b32 bold, Vec4F32 fg)
{
  Vec2F32 result = {0};

  Fnt_Metrics metrics = fnt_metrics(font, size);
  u32 missing_char = get_glyph_index(font, 0xFFFD);

  u32 space_char = get_glyph_index(font, ' ');
  f32 space_width = rasterize_glyph(font, size, space_char, false, 0).advance;
  b32 should_draw = fg.w > 1e-5f;

  f32 x = floorf(pos.x);
  f32 y = floorf(pos.y);

  s32 incr;
  f32 tab_size_px = (f32) tab_width * space_width;
  u32 prev_glyph = 0;
  for (s64 i=0; i<text.len; i += (s64) Max(incr, 1))
  {
    u32 codep = utf8_decode(text, i, &incr);

    switch (codep)
    {
      case ' ':
      {
        x += space_width;
        prev_glyph = 0;
      } break;
      case '\t':
      {
        f32 rel = x - pos.x;
        f32 next_stop = (floorf(rel / tab_size_px) + 1.f) * tab_size_px;
        x = pos.x + next_stop;
        prev_glyph = 0;
      } break;
      case '\n':
      {
        x = pos.x;
        y += metrics.line_height;
        result.y += metrics.line_height;
        prev_glyph = 0;
      } break;
      default:
      {
        u32 glyph_index = get_glyph_index(font, codep);
        glyph_index = (glyph_index == 0) ? missing_char : glyph_index;

        if (prev_glyph != 0)
        {
          x -= get_kerning(font, size, prev_glyph, glyph_index);
        }
        prev_glyph = glyph_index;

        Glyph_Rect glyph = fnt_get_glyph(font, size, bold, glyph_index);
        if (should_draw) 
        {
          Rng2S16 rect = glyph.rect;
          f32 w = (rect.max.x - rect.min.x);
          f32 h = (rect.max.y - rect.min.y);
          Vec2F32 dst_min = { x + glyph.offset.x, y - glyph.offset.y + metrics.ascent };
          Rng2F32 dst = { 
            .min = dst_min,
            .max = vec2_add(dst_min, v2f32(w, h))
          };
          Rng2F32 src = {
            .min = vec2_scale(v2f32(rect.min.x, rect.min.y), 1.f / R_FNT_ATLAS_DIM),
            .max = vec2_scale(v2f32(rect.max.x, rect.max.y), 1.f / R_FNT_ATLAS_DIM),
          };
          Quad_Text text_quad = {
            .dst = dst,
            .src = src,
            .fg = fg,
          };
          push_text_quad_(list, fnt_state->texture, &text_quad);
        }
        x += glyph.advance;
        result.x = Max(result.x, x - pos.x);
      } break;
    }
  }

  if (text.raw[text.len -1] != '\n')
  {
    result.y += metrics.line_height;
  }
  return result;
}

