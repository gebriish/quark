#ifndef DRAW_H
#define DRAW_H

#include "gfx.h"
#include "glyph_cache.h"

internal void draw_cursor(vec2 pos, vec2 size, color8_t color, u32 texture);
internal void draw_quad(vec2 pos, vec2 size, color8_t color);
internal void draw_quad_textured(vec2 pos, vec2 size, color8_t color, u32 texture);
internal void draw_string(String8 string, vec2 position, color8_t color, int tab_width, Glyph_Cache *cache);

#endif
