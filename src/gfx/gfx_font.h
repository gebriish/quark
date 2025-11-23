#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "../base/base_hash.h"
#include "../base/base_string.h"

#include "gfx_core.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb/stb_truetype.h"

#define FONT_CHARSET str8_lit(\
"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" \
"ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ" \
"ΆΈΉΊΌΎΏΪΫάέήίΰαβγδεζηθικλμνξοπρστυφχψωϊϋόύώ"\
)

typedef struct Font Font;
struct Font {
	stbtt_fontinfo font_info;
	f32 scale;
	f32 ascent;
	f32 descent;
	f32 line_gap;
};

typedef struct Glyph_Info Glyph_Info;
struct Glyph_Info {
	rune codepoint;
	i32 x0, y0, x1, y1;
	f32 xoff, yoff;
	f32 xadvance;
};

Map32_Define(glyph_map, Glyph_Info);

typedef struct Font_Atlas Font_Atlas;
struct Font_Atlas {
	u8 *atlas_data;
	glyph_map *glyphs;
	Font font;
	u32 tex_id;
	i32 atlas_width, atlas_height;
	i32 current_x, current_y;
	i32 row_height;
	bool dirty;
};

internal bool font_atlas_new(Allocator *allocator, u8 *font_data,	i32 size, f32 font_height, Font_Atlas *result);
internal void font_atlas_delete(Allocator *allocato, Font_Atlas *atlas);

internal bool font_atlas_add_glyph(Allocator *allocator, Font_Atlas *atlas, rune codepoint);
internal bool font_atlas_expand(Allocator *allocator, Font_Atlas *atlas);

internal void font_atlas_add_glyphs_from_string(Allocator *allocator, Font_Atlas *atlas, String8 string);
internal void font_atlas_update(Font_Atlas *atlas);

internal bool font_atlas_get_glyph(Font_Atlas *atlas, rune codepoint, Glyph_Info *out_glyph);
internal bool font_atlas_resize_glyphs(Allocator *allocator, Font_Atlas *atlas, f32 new_font_height);

internal f32 font_atlas_height(Font_Atlas *atlas);

#endif
