#include "gfx_font.h"

global Font_State *_font_state = NULL;

internal void
font_init(Arena *allocator, String8 path)
{
	_font_state = arena_push_struct(allocator, Font_State);

	FT_Init_FreeType(&_font_state->library);


	Temp t;
	DeferScope(t= temp_begin(allocator), temp_end(t)) {
		FT_Face face = 0;
		FT_New_Face(
			_font_state->library, 
			(char *)str8_to_ctring(allocator, path).raw,
			0,
			&face
		);
		_font_state->face = face;
	}
}

internal void
font_close()
{
	Assert(_font_state && _font_state->face);

	FT_Done_Face(_font_state->face);
	FT_Done_FreeType(_font_state->library);
	_font_state = NULL;
}

internal Font_Metrics
font_get_metrics(u32 font_size)
{
	Assert(_font_state && _font_state->face);
	FT_Face face = _font_state->face;

	FT_Set_Pixel_Sizes(face, 0, font_size);

	Font_Metrics result = {0};
	result.font_size = font_size;

	f32 scale = (f32)font_size / (f32)face->units_per_EM;

	result.design_units_per_em = (f32)(face->units_per_EM);
	result.ascent              = (f32) face->ascender;
	result.descent             = -(f32)face->descender;
	result.line_gap            = (f32)(face->height - face->ascender + face->descender);

	return result;
}

internal Font_Atlas
font_generate_atlas(Arena *allocator, u32 font_size, String8 characters)
{
	Assert(_font_state && _font_state->face);
	FT_Face face = _font_state->face;
	FT_Set_Pixel_Sizes(face, 0, font_size);

	u32 padding = 1;
	u32 atlas_width = 512;
	u32 atlas_height = 512;

	usize codepoint_count = 0;
	u32 x = padding, y = padding, max_row_height = 0;

	// First pass: calculate atlas height
	str8_foreach(characters, itr, i) {
		if (FT_Load_Char(face, itr.codepoint, FT_LOAD_RENDER)) continue;
		FT_GlyphSlot g = face->glyph;

		u32 glyph_width = g->bitmap.width;
		if (x + glyph_width + padding > atlas_width) {
			x = padding;
			y += max_row_height + padding;
			max_row_height = 0;
		}
		x += glyph_width + padding;
		max_row_height = Max(max_row_height, g->bitmap.rows);
		codepoint_count += 1;
	}
	atlas_height = y + max_row_height + padding;

	glyph_map *code_to_glyph = glyph_map_new(allocator, codepoint_count);

	// Second pass: store glyph info
	x = padding; y = padding; max_row_height = 0;
	str8_foreach(characters, itr, i) {
		u32 codepoint = itr.codepoint;
		if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) continue;
		FT_GlyphSlot g = face->glyph;

		u32 glyph_width = g->bitmap.width;
		if (x + glyph_width + padding > atlas_width) {
			x = padding;
			y += max_row_height + padding;
			max_row_height = 0;
		}

		Glyph_Info info = {
			.position = {(i16)x, (i16)y},
			.size = {(u8)glyph_width, (u8)g->bitmap.rows},
			.bearing = {(i8)g->bitmap_left, (i8)g->bitmap_top},
			.advance = (i16)(g->advance.x >> 6)
		};
		glyph_map_insert(code_to_glyph, codepoint, info);

		x += glyph_width + padding;
		max_row_height = Max(max_row_height, g->bitmap.rows);
	}

	Font_Metrics metrics = font_get_metrics(font_size);
	Font_Atlas atlas = {
		.dim = {(u16)atlas_width, (u16)atlas_height},
		.code_to_glyph = code_to_glyph,
		.metrics = metrics
	};

	return atlas;
}

internal u8 *
font_rasterize_atlas(Arena *scratch, Font_Atlas *atlas)
{
	Assert(_font_state && _font_state->face);
	FT_Face face = _font_state->face;
	FT_Set_Pixel_Sizes(face, 0, atlas->metrics.font_size);

	u32 atlas_width  = atlas->dim.x;
	u32 atlas_height = atlas->dim.y;
	u8 *atlas_image  = arena_push_array_zeroed(scratch, u8, atlas_width * atlas_height);

	glyph_map *map = atlas->code_to_glyph;
	for (int i = 0; i < map->capacity; ++i) {
		glyph_map_Entry *entry = &map->entries[i];
		if (!entry->occupied) continue;

		rune codepoint = (rune)entry->key;
		Glyph_Info *info = &entry->value;

		if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) continue;
		FT_GlyphSlot g = face->glyph;
		FT_Bitmap *bmp = &g->bitmap;

		u32 dst_x = (u32)info->position.x;
		u32 dst_y = (u32)info->position.y;

		u8 *src = bmp->buffer;
		for (u32 row = 0; row < bmp->rows; ++row) {
			u8 *src_row = src + row * (u32)bmp->pitch;
			u8 *dst_row = atlas_image + (dst_y + row) * atlas_width + dst_x;
			MemCopy(dst_row, src_row, bmp->width);
		}
	}

	return atlas_image;
}
