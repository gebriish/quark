#include "gfx_font.h"


internal_lnk void
font_init(String8 path)
{
  Arena *arena = arena_alloc(MB(5));
  fnt_state = arena_push_struct(arena, Font_State);
  fnt_state->arena = arena;
  FT_Init_FreeType(&fnt_state->library);

  Temp scratch = temp_begin(arena);
  String8 c_path = str8_cstr_copy(arena, path);
  FT_Face face = 0;
  FT_New_Face(fnt_state->library, (char *)c_path.raw, 0, &face);
  temp_end(scratch);

  fnt_state->face = face;
}


internal_lnk void
font_close()
{
  Assert(fnt_state && fnt_state->face);
  FT_Done_Face(fnt_state->face);
  FT_Done_FreeType(fnt_state->library);

  arena_release(fnt_state->arena);
  fnt_state = 0;
}


internal_lnk Font_Metrics
font_get_metrics()
{
  Assert(fnt_state && fnt_state->face);

  Font_Metrics result = {0};
  FT_Face face = fnt_state->face;

  result.design_units_per_em =  (f32)(face->units_per_EM);
  result.ascent              =  (f32)(face->ascender);
  result.descent             = -(f32)(face->descender);
  result.line_gap            =  (f32)(face->height - face->ascender + face->descender);
  result.capital_height      =  (f32)(face->ascender);
  
  return result;
}


internal_lnk void
font_generate_atlas(u32 font_size, String8 characters)
{
  Assert(fnt_state && fnt_state->face);

  FT_Face face = fnt_state->face;

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
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) continue;

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

  u8 *atlas_image = arena_push_array_zeroed(fnt_state->arena, u8, atlas_width * atlas_height);

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

  fnt_state->atlas = atlas;
}
