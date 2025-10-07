#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "../base/base_core.h"
#include "../base/base_string.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

typedef struct {
  f32 design_units_per_em;
  f32 ascent;
  f32 descent;
  f32 line_gap;
  f32 capital_height;
} Font_Metrics;

typedef struct Glyph_Info Glyph_Info;
struct Glyph_Info {
  vec2_i16 position;
  vec2_i16 size;
  vec2_i16 bearing;
  i16 advance;
};

typedef struct Font_Atlas Font_Atlas;
struct Font_Atlas {
  u8 *image;
  vec2_u16 dim;
};

typedef struct Font_State Font_State;
struct Font_State {
  FT_Library library;
  FT_Face face;
  Font_Atlas atlas;
};

internal_lnk void font_init(Arena *allocator, String8 path);
internal_lnk void font_close();

internal_lnk Font_Metrics font_get_metrics();
internal_lnk Font_Atlas   font_generate_atlas(Arena *allocator, u32 font_size, String8 characters);

#endif
