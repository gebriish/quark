#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "../base/base_core.h"
#include "../base/base_string.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

Enum(u32, Font_RasterFlag) {
  Font_RasterSmooth = (1 << 0),
  Font_RasterHint   = (1 << 1),
};

typedef struct {
  f32 design_units_per_em;
  f32 ascent;
  f32 descent;
  f32 line_gap;
  f32 capital_height;
} Font_Metrics;

typedef struct Font_RasterResult Font_RasterResult;
struct Font_RasterResult
{
  vec2_i16 atlas_dim;
  void *atlas;
  f32 advance;
};

typedef struct Font_State Font_State;
struct Font_State {
  Arena *arena;
  FT_Library library;
  FT_Face face;
};

global Font_State *fnt_state = NULL;

internal_lnk void font_init(String8 path);

#endif
