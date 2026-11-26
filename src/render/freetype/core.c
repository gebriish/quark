#define R_FNT_BUCKET_COUNT 16

function void
fnt_provider_init(void)
{
  Arena *arena = arena_reserve(Megabytes(16));
  r_ft_state = arena_push(arena, R_FT_State, 1);
  r_ft_state->arena = arena;
  FT_Init_FreeType(&r_ft_state->ft_lib);
  FT_Library_SetLcdFilter(r_ft_state->ft_lib, FT_LCD_FILTER_DEFAULT);
  r_ft_state->fonts = arena_push(arena, R_FT_Bucket, R_FNT_BUCKET_COUNT);
}

function Fnt_Handle
fnt_alloc(String8 data, Font_Alloc_Flags flags)
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
    if (r_ft_state->free_fonts)
    {
      font = r_ft_state->free_fonts;
      SLLStackPop_N(r_ft_state->free_fonts, free_next);
      MemZeroStruct(font);
    }
    else
    {
      font = arena_push(r_ft_state->arena, R_FT_Font, 1);
    }

    font->hash = hash;

    if (flags & Font_Alloc_Flag_NoCopy)
    {
      font->data      = data;
      font->owns_data = 0;
    }
    else
    {
      R_FT_FreeNode *node = 0;
      for each_node(itr, R_FT_FreeNode, r_ft_state->data_free_list.first)
      {
        if (itr->size >= data.len)
        {
          node = itr;
          DLLRemove(r_ft_state->data_free_list.first, r_ft_state->data_free_list.last, itr);
          r_ft_state->data_free_list.len -= 1;
          break;
        }
      }

      if (node == 0)
      {
        font->data = str8_push_copy(r_ft_state->arena, data);
        font->data_cap = data.len;
      }
      else
      {
        font->data     = str8((u8 *) node, data.len);
        font->data_cap = node->size;
        MemMove(font->data.raw, data.raw, data.len);
      }
      font->owns_data = true;
    }

    FT_Error err = FT_New_Memory_Face(r_ft_state->ft_lib, font->data.raw,
                                      (FT_Long) font->data.len, 0, &font->face);
    if (err != 0)
    {
      r_ft_fnt_release_font_storage(font);
      return (Fnt_Handle) {0};
    }

    DLLPushBack_N(r_ft_state->fonts[index].first, r_ft_state->fonts[index].last,
                  font, hash_next, hash_prev);
  }

  font->ref_count += 1;

  Fnt_Handle result = {0};
  result.v[0] = (u64) font;
  return result;
}

function void
fnt_free(Fnt_Handle font)
{
  R_FT_Font *ft_font = (R_FT_Font *) font.v[0];
  if (ft_font == 0) return;

  ft_font->ref_count -= 1;
  if (ft_font->ref_count <= 0)
  {
    u64 index = ft_font->hash & (R_FNT_BUCKET_COUNT - 1);
    DLLRemove_N(r_ft_state->fonts[index].first,
                r_ft_state->fonts[index].last,
                ft_font, hash_next, hash_prev);
    FT_Done_Face(ft_font->face);

    r_ft_fnt_release_font_storage(ft_font);
  }
}

function void 
r_ft_fnt_release_font_storage(R_FT_Font *font)
{
  if (font->owns_data && font->data.raw != 0)
  {
    R_FT_FreeNode *node = (R_FT_FreeNode *) font->data.raw;
    node->size = font->data_cap;
    DLLPushBack(r_ft_state->data_free_list.first, r_ft_state->data_free_list.last, node);
  }
  font->data      = (String8) {0};
  font->data_cap  = 0;
  font->owns_data = 0;
  SLLStackPush_N(r_ft_state->free_fonts, font, free_next);
}

function u32
get_glyph_index(Fnt_Handle font, u32 codepoint)
{
  R_FT_Font *f = (R_FT_Font *) font.v[0];
  return FT_Get_Char_Index(f->face, codepoint);
}

function Fnt_Metrics
fnt_metrics(Fnt_Handle font, u32 size)
{
  R_FT_Font *f = (R_FT_Font *) font.v[0];
  FT_Set_Pixel_Sizes(f->face, 0, (FT_UInt) (size));
  FT_Size_Metrics *m = &f->face->size->metrics;

  Fnt_Metrics result = {
    .ascent      =  (f32) m->ascender  / 64.f,
    .descent     = -(f32) m->descender / 64.f,
    .line_height =  (f32) m->height    / 64.f,
  };

  return result;
}

function Font_Raster 
rasterize_glyph(Fnt_Handle font, u32 size, u32 index, b32 bold, Arena *arena)
{
  R_FT_Font *f = (R_FT_Font *) font.v[0];
  FT_Set_Pixel_Sizes(f->face, 0, (FT_UInt) size);

  Font_Raster result = {0};

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
    result.dim.x   = (s32) bmp->width / 3;
    result.dim.y   = (s32) bmp->rows;
    result.advance = (f32) (slot->advance.x >> 6);
    result.offset  = v2s16((s16) slot->bitmap_left, (s16) slot->bitmap_top);

    if (arena && bmp->buffer && bmp->rows)
    {
      result.data = arena_push(arena, u8, (s64) bmp->width * bmp->rows);

      for each_index(y, bmp->rows)
      {
        MemMove(result.data + (s64) y * bmp->width,
                bmp->buffer + (s64) y * bmp->pitch,
                bmp->width);
      }
    }
  }
  return result;
}

function f32
get_kerning(Fnt_Handle font, u32 size, u32 left, u32 right)
{
  R_FT_Font *f = (R_FT_Font *) font.v[0];
  FT_Set_Pixel_Sizes(f->face, 0, (FT_UInt) size);

  FT_Vector delta = {0};
  FT_Get_Kerning(f->face, left, right, FT_KERNING_DEFAULT, &delta);
  return (f32) delta.x / 64.f;
}

