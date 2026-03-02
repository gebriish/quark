#ifndef DRAW_H
#define DRAW_H

#include "gfx.h"
#include "glyph_cache.h"

typedef u16 Align_H;
enum {
	AlignH_Left,
	AlignH_Center,
	AlignH_Right,
};

typedef u16 Align_V;
enum {
	AlignV_Top,
	AlignV_Center,
	AlignV_Bottom,
};

typedef struct {
	Align_H h;
	Align_V v;
} Box_Alignment;

internal void draw_cursor(vec2 pos, vec2 size, color8_t color, u32 texture);
internal void draw_quad(vec2 pos, vec2 size, color8_t color);
internal void draw_quad_textured(vec2 pos, vec2 size, color8_t color, u32 texture);
internal void draw_string(String8 string, vec2 position, color8_t color, int tab_width, Glyph_Cache *cache);
internal void draw_string_aligned(String8 string, vec2 position, vec2 box_size, color8_t color, int tab_width, Box_Alignment alignment, Glyph_Cache *cache);

#endif
