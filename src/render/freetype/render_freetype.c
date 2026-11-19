#define R_FNT_BUCKET_COUNT 16

// it should be a power of 2, I am using & ( x - 1 ) for modulo

function void
r_fnt_provider_init(void)
{
  Arena *arena = arena_reserve(Kilobytes(16));
  r_ft_state = arena_push(arena, R_FT_State, 1);
  r_ft_state->arena = arena;
  FT_Init_FreeType(&r_ft_state->ft_lib);
  FT_Library_SetLcdFilter(r_ft_state->ft_lib, FT_LCD_FILTER_DEFAULT);
  r_ft_state->fonts = arena_push(arena, R_FT_Bucket, R_FNT_BUCKET_COUNT);
}

function R_Font 
r_fnt_alloc(String8 data, R_FontAllocFlags flags)
{
  u64 hash  = str8_hash(data);
  u64 index = hash & (R_FNT_BUCKET_COUNT - 1);

  R_FT_Font *font = 0;

  for (R_FT_Font *node = r_ft_state->fonts[index].first; node != 0; node = node->hash_next)
  {
    if (node->hash == hash && str8_matches(node->data, data))
    {
      font = node;
      break;
    }
  }

  if (font == 0)
  {
    font = r_ft_state->free_fonts;
    if (r_ft_state->free_fonts)
    {
      SLLStackPop_N(r_ft_state->free_fonts, free_next);
      MemZeroStruct(font);
    }
    else
    {
      font = arena_push(r_ft_state->arena, R_FT_Font, 1);
    }

    font->hash  = hash;

    if (flags & R_FontAllocFlag_NoCopy)
    {
      font->data = data;
    }
    else
    {
      font->data_arena = arena_reserve(align_up_power2(data.len, Kilobytes(4)));
      font->data       = str8_push_copy(font->data_arena, data);
    }

    FT_Error err = FT_New_Memory_Face(r_ft_state->ft_lib, font->data.raw, (FT_Long) font->data.len, 0, &font->face);
    DLLPushBack_N(r_ft_state->fonts[index].first, r_ft_state->fonts[index].last, font, hash_next, hash_prev);
  }

  font->ref_count += 1;

  R_Font result = {0};
  result.v[0] = (u64) font;
  return result;
}

function void
r_fnt_free(R_Font font)
{
  R_FT_Font *ft_font = (R_FT_Font *) font.v[0];
  if (ft_font == 0) return;

  ft_font->ref_count -= 1;
  if (ft_font->ref_count <= 0)
  {
    u64 index = ft_font->hash & (R_FNT_BUCKET_COUNT - 1);
    DLLRemove_N( r_ft_state->fonts[index].first,
                 r_ft_state->fonts[index].last,
                 ft_font, hash_next, hash_prev );
    FT_Done_Face(ft_font->face);
    arena_release(ft_font->data_arena);
    SLLStackPush_N(r_ft_state->free_fonts, ft_font, free_next);
  }
}

function u32
r_fnt_glyph_index(R_Font font, u32 codepoint)
{
  R_FT_Font *f = (R_FT_Font *) font.v[0];
  return FT_Get_Char_Index(f->face, codepoint);
}

function R_FontMetrics
r_fnt_metrics(R_Font font, u32 size)
{
  R_FT_Font *f = (R_FT_Font *) font.v[0];
  FT_Set_Pixel_Sizes(f->face, 0, (FT_UInt) (size));
  FT_Size_Metrics *m = &f->face->size->metrics;

  R_FontMetrics result = {
    .ascent      =  m->ascender  / 64.f,
    .descent     = -m->descender / 64.f,
    .line_height =  m->height    / 64.f,
  };

  return result;
}

function R_FontRaster 
r_fnt_raster(R_Font font, u32 size, u32 index, b32 bold, Arena *arena)
{
  R_FT_Font *f = (R_FT_Font *) font.v[0];
  FT_Set_Pixel_Sizes(f->face, 0, (FT_UInt) size);

  R_FontRaster result = {0};

  if (FT_Load_Glyph(f->face, index, FT_LOAD_TARGET_LCD) != 0) return result;

  if (bold)
  {
    FT_GlyphSlot_Embolden(f->face->glyph);
  }

  if (FT_Render_Glyph(f->face->glyph, FT_RENDER_MODE_LCD) != 0) return result;

  FT_GlyphSlot slot = f->face->glyph;
  FT_Bitmap   *bmp  = &slot->bitmap;

  if (bmp->pixel_mode == FT_PIXEL_MODE_LCD)
  {
    result.dim.x   = bmp->width / 3;
    result.dim.y   = bmp->rows;
    result.advance = (f32) (slot->advance.x >> 6);
    result.offset  = (Vec2S16) { slot->bitmap_left, slot->bitmap_top };

    if (arena && bmp->buffer && bmp->rows)
    {
      result.data = arena_push(arena, u8, (s64) bmp->width * bmp->rows);

      for EachIndex(y, bmp->rows)
      {
        MemMove(result.data + (s64) y * bmp->width,
                bmp->buffer + (s64) y * bmp->pitch,
                bmp->width);
      }
    }
  }
  return result;
}

