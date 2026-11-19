
const S64 R_FNT_ATLAS_DIM = 512;

function B32 
r_fnt_zero(R_Font f)
{
  return (f.u64[0] == 0);
}

function B32 
r_fnt_matches(R_Font a, R_Font b)
{
  return (a.u64[0] == b.u64[0]);
}

//
// font glyph caching
//

function void
r_fnt_init(void)
{
  Arena *arena = arena_reserve(Megabytes(4));
  r_fnt_state = arena_push(arena, R_FontState, 1);
  r_fnt_state->arena = arena;

  r_fnt_state->texture = r_tex_alloc(r_blank_image(R_TexFormat_RGB8, (Vec2S32) { R_FNT_ATLAS_DIM, R_FNT_ATLAS_DIM }));

  //  initialize the font provider backend
  r_fnt_provider_init();
}

function R_Glyph 
r_fnt_get_glyph(R_Font font, U32 size, B32 bold, U32 index)
{
  U128 key = {0};

  key.lo = font.u64[0];
  key.hi = (U64) size << 32 | (U64) index | (U64) bold << 63;

  U64 hash = u128_hash(key);
  U64 hash_index = hash & (CArrayLength(r_fnt_state->slots) - 1);

  R_GlyphNode *glyph_node = 0;
  R_GlyphHashBucket bucket = r_fnt_state->slots[hash_index];
  for (R_GlyphNode *node = bucket.first; node; node = node->hash_next)
  {
    if (u128_matches(node->key, key)) 
    {
      glyph_node = node;
      break;
    }
  }

  if (!glyph_node)
  {
    glyph_node = r_fnt_state->free_glyphs;
    if (r_fnt_state->free_glyphs)
    {
      SLLStackPop_N(r_fnt_state->free_glyphs, free_next);
      MemZeroStruct(glyph_node);
    }
    else
    {
      glyph_node = arena_push(r_fnt_state->arena, R_GlyphNode, 1);
    }

    R_Glyph glyph = {0};
    ScratchScope(scratch, 0, 0)
    {
      R_FontRaster img = r_fnt_raster(font, size, index, bold, scratch.arena);

      if (r_fnt_state->cursor.x + img.dim.x >= R_FNT_ATLAS_DIM)
      {
        r_fnt_state->cursor.x = 0;
        r_fnt_state->cursor.y += r_fnt_state->row_height + 1;
      }

      Rng2S32 range = {
        .min = r_fnt_state->cursor,
        .max = { r_fnt_state->cursor.x + img.dim.x, r_fnt_state->cursor.y + img.dim.y },
      };

      r_tex_update(r_fnt_state->texture, range, img.data);
      r_fnt_state->cursor.x += img.dim.x + 1;
      r_fnt_state->row_height = Max(r_fnt_state->row_height, img.dim.y);

      glyph.offset = img.offset;
      glyph.advance = img.advance;
      glyph.rect = (Rng2S16) {
        .min = { range.min.x, range.min.y },
        .max = { range.max.x, range.max.y },
      };
    }

    glyph_node->glyph = glyph;
    glyph_node->key = key;
    DLLPushBack_N(r_fnt_state->slots[hash_index].first,
                  r_fnt_state->slots[hash_index].last,
                  glyph_node, hash_next, hash_prev);
  }

  return glyph_node->glyph;
}

function void 
r_fnt_frame(void)
{
  
}

/////////////////////////////
// @section: pushing draw commands

function Vec2F32 
r_push_text_simple(R_DrawList *list, R_Font font, String8 text, Vec2F32 pos, U32 size, S32 tab_width, B32 bold, Vec4F32 fg)
{
  Vec2F32 result = {0};

  R_FontMetrics metrics = r_fnt_metrics(font, size);
  U32 missing_char = r_fnt_glyph_index(font, 0xFFFD);

  U32 space_char = r_fnt_glyph_index(font, ' ');
  F32 space_width = r_fnt_raster(font, size, space_char, false, 0).advance;
  B32 should_draw = fg.w > 1e-5f;

  F32 x = floorf(pos.x);
  F32 y = floorf(pos.y);

  S32 incr;
  F32 tab_size_px = tab_width * space_width;
  for (S64 i=0; i<text.len; i += (S64) incr)
  {
    U32 codep = utf8_decode(text, i, &incr);

    switch (codep)
    {
      case ' ':
      {
        x += space_width;
      } break;
      case '\t':
      {
        F32 rel = x - pos.x;
        F32 next_stop = (floorf(rel / tab_size_px) + 1.f) * tab_size_px;
        x = pos.x + next_stop;
      } break;
      case '\n':
      {
        x = pos.x;
        y += metrics.line_height;
        result.y += metrics.line_height;
      } break;
      default:
      {
        U32 glyph_index = r_fnt_glyph_index(font, codep);
        glyph_index = (glyph_index == 0) ? missing_char : glyph_index;
        R_Glyph glyph = r_fnt_get_glyph(font, size, bold, glyph_index);

        if (should_draw) 
        {
          Rng2S16 rect = glyph.rect;
          F32 w = (rect.max.x - rect.min.x);
          F32 h = (rect.max.y - rect.min.y);
          Vec2F32 dst_min = { x + glyph.offset.x, y - glyph.offset.y + metrics.ascent };
          Rng2F32 dst = { 
            .min = dst_min,
            .max = vec2_add(dst_min, V2(w, h))
          };
          Rng2F32 src = {
            .min = vec2_scale(V2(rect.min.x, rect.min.y), 1.f / R_FNT_ATLAS_DIM),
            .max = vec2_scale(V2(rect.max.x, rect.max.y), 1.f / R_FNT_ATLAS_DIM),
          };
          R_TextQuad text_quad = {
            .dst = dst,
            .src = src,
            .fg = fg,
          };
          r_push_text_quad_(list, r_fnt_state->texture, &text_quad);
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

