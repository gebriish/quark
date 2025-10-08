#include "gfx_font.h"

global Font_State *_font_state = NULL;

internal_lnk void
font_init(Arena *allocator, String8 path)
{
  _font_state = arena_push_struct(allocator, Font_State);
  FT_Init_FreeType(&_font_state->library);
  Temp scratch = temp_begin(allocator);
  String8 c_path = str8_cstr_copy(allocator, path);
  FT_Face face = 0;
  FT_New_Face(_font_state->library, (char *)c_path.raw, 0, &face);
  temp_end(scratch);
  _font_state->face = face;
}

internal_lnk void
font_close()
{
  Assert(_font_state && _font_state->face);
  FT_Done_Face(_font_state->face);
  FT_Done_FreeType(_font_state->library);
  _font_state = NULL;
}

internal_lnk Font_Metrics
font_get_metrics(u32 font_size)
{
  Assert(_font_state && _font_state->face);
  FT_Face face = _font_state->face;
  
  FT_Set_Pixel_Sizes(face, 0, font_size);
  
  Font_Metrics result = {0};
  result.font_size = font_size;
  
  f32 scale = (f32)font_size / (f32)face->units_per_EM;
  
  result.design_units_per_em = (f32)face->units_per_EM;
  result.ascent = (f32)face->ascender * scale;
  result.descent = -(f32)face->descender * scale;
  result.line_gap = (f32)(face->height - face->ascender + face->descender) * scale;
  result.capital_height = result.ascent; // approximation
  
  return result;
}

internal_lnk Font_Atlas
font_generate_atlas(Arena *allocator, Arena *scratch, u32 font_size, String8 characters)
{
  Assert(_font_state && _font_state->face);
  FT_Face face = _font_state->face;
  FT_Set_Pixel_Sizes(face, 0, font_size);
  
  u32 padding = 1;
  u32 atlas_width = 512;
  u32 atlas_height = 512;
  
  usize codepoint_count = 0;
  u32 x = padding, y = padding, max_row_height = 0;
  str8_foreach(characters, itr, i) {
    if (FT_Load_Char(face, itr.codepoint, FT_LOAD_DEFAULT)) continue;
    FT_GlyphSlot g = face->glyph;
    
    if (x + g->bitmap.width + padding > atlas_width) {
      x = padding;
      y += max_row_height + padding;
      max_row_height = 0;
    }
    x += g->bitmap.width + padding;
    max_row_height = Max(max_row_height, g->bitmap.rows);
    codepoint_count += 1;
  }
  atlas_height = y + max_row_height + padding;
  
  u8 *atlas_image = arena_push_array_zeroed(scratch, u8, atlas_width * atlas_height);
  glyph_map *code_to_glyph = glyph_map_new(allocator, codepoint_count);
  
  x = padding, y = padding, max_row_height = 0;
  str8_foreach(characters, itr, i) {
    u32 codepoint = itr.codepoint;
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) continue;
    FT_GlyphSlot g = face->glyph;
    
    if (x + g->bitmap.width + padding > atlas_width) {
      x = padding;
      y += max_row_height + padding;
      max_row_height = 0;
    }
    
    for (u32 row = 0; row < g->bitmap.rows; ++row) {
      u8 *dst = &atlas_image[(y + row) * atlas_width + x];
      u8 *src = &g->bitmap.buffer[row * (u32)g->bitmap.pitch];
      MemCopy(dst, src, g->bitmap.width);
    }
    
    Glyph_Info info = {
      .position = {(i16)x, (i16)y},
      .size = {(u8)g->bitmap.width, (u8)g->bitmap.rows},
      .bearing = {(i8)g->bitmap_left, (i8)g->bitmap_top},
      .advance = (i16)(g->advance.x >> 6)
    };
    glyph_map_insert(code_to_glyph, codepoint, info);
    
    x += g->bitmap.width + padding;
    max_row_height = Max(max_row_height, g->bitmap.rows);
  }
  
  Font_Metrics metrics = font_get_metrics(font_size);
  
  Font_Atlas atlas = {
    .image = atlas_image,
    .dim = {(u16)atlas_width, (u16)atlas_height},
    .code_to_glyph = code_to_glyph,
    .metrics = metrics
  };
  
  return atlas;
}
