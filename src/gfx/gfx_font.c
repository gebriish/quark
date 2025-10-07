#include "gfx_font.h"

global Font_State *
  _font_state = NULL;

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
font_get_metrics()
{
  Assert(_font_state && _font_state->face);

  Font_Metrics result = {0};
  FT_Face face = _font_state->face;

  result.design_units_per_em =  (f32)(face->units_per_EM);
  result.ascent              =  (f32)(face->ascender);
  result.descent             = -(f32)(face->descender);
  result.line_gap            =  (f32)(face->height - face->ascender + face->descender);
  result.capital_height      =  (f32)(face->ascender);
  
  return result;
}

internal_lnk Font_Atlas
font_generate_atlas(Arena *allocator, u32 font_size, String8 characters)
{
  Assert(_font_state && _font_state->face);

  FT_Face face = _font_state->face;

  FT_Set_Pixel_Sizes(face, 0, font_size);

  u32 padding = 1;
  u32 row_height = 0;
  u32 atlas_width = 512;
  u32 atlas_height = 512;
  u32 x = padding;
  u32 y = padding;

  u32 max_row_height = 0;
  str8_foreach(characters, itr, i) {
    u32 codepoint = itr.codepoint;
    if (FT_Load_Char(face, codepoint, FT_LOAD_DEFAULT)) continue;

    FT_GlyphSlot g = face->glyph;
    if (x + g->bitmap.width + padding > atlas_width) {
      x = padding;
      y += max_row_height + padding;
      max_row_height = 0;
    }
    x += g->bitmap.width + padding;
    if (g->bitmap.rows > max_row_height) max_row_height = g->bitmap.rows;
  }
  atlas_height = y + max_row_height + padding;

  u8 *atlas_image = arena_push_array_zeroed(allocator, u8, atlas_width * atlas_height);

  x = padding;
  y = padding;
  max_row_height = 0;

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
      for (u32 col = 0; col < g->bitmap.width; ++col) {
        atlas_image[(y + row) * atlas_width + (x + col)] =
          g->bitmap.buffer[row * g->bitmap.pitch + col];
      }
    }

    x += g->bitmap.width + padding;
    if (g->bitmap.rows > max_row_height) max_row_height = g->bitmap.rows;
  }

  Font_Atlas atlas = {0};
  atlas.image = atlas_image;
  atlas.dim = (vec2_u16){.x = (u16)atlas_width, .y = (u16)atlas_height};
  return atlas;
}
