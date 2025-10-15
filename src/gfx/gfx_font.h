#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "../base/base_core.h"
#include "../base/base_hash.h"
#include "../base/base_string.h"

#undef internal
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#define internal static

typedef struct Glyph_Info Glyph_Info;
struct Glyph_Info {
	vec2_i16 position;
	i16 advance;
	vec2_u8 size;
	vec2_i8 bearing;
};

Map32_Define(glyph_map, Glyph_Info)

typedef struct {
	f32 design_units_per_em;
	f32 ascent;
	f32 descent;
	f32 line_gap;
	u32 font_size;
} Font_Metrics;

typedef struct Font_Atlas Font_Atlas;
struct Font_Atlas {
	glyph_map *code_to_glyph;
	vec2_u16 dim;
	Font_Metrics metrics;
};

typedef struct Font_State Font_State;
struct Font_State {
	FT_Library library;
	FT_Face face;
};

internal void font_init(Arena *allocator, String8 path);
internal void font_close();

internal Font_Metrics font_get_metrics(u32 font_size);

internal Font_Atlas font_generate_atlas(Arena *allocator, u32 font_size, String8 characters);
internal u8 *font_rasterize_atlas(Arena *scratch, Font_Atlas *atlas);

internal force_inline f32
font_metrics_line_height(Font_Metrics *metrics)
{
	return metrics->ascent + metrics->descent + metrics->line_gap;
}

internal force_inline f32
font_metrics_baseline_offset(Font_Metrics *metrics)
{
	return metrics->ascent;
}

internal force_inline f32
font_metrics_scale_factor(Font_Metrics *metrics, f32 desired_size)
{
	return desired_size / (f32)metrics->font_size;
}

#define FONT_CHARSET \
"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" \
"ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ" \
"ΆΈΉΊΌΎΏΪΫάέήίΰαβγδεζηθικλμνξοπρστυφχψωϊϋόύώ" \

#endif
