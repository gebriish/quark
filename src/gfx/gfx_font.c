#include "gfx_font.h"

internal bool
font_atlas_new(Arena *allocator, u8 *font_data, i32 size, f32 font_height, Font_Atlas *atlas)
{
	Assert(atlas && font_data);
	
	if (!stbtt_InitFont(&atlas->font.font_info, font_data, 0)) {
		LogError("Failed to Initialize STB Font Info");
		return false;
	}
	atlas->font.scale = stbtt_ScaleForPixelHeight(&atlas->font.font_info, font_height);
	
	i32 ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&atlas->font.font_info, &ascent, &descent, &line_gap);
	
	atlas->font.ascent   = (f32)(ascent)   * atlas->font.scale;
	atlas->font.descent  = (f32)(descent)  * atlas->font.scale;
	atlas->font.line_gap = (f32)(line_gap) * atlas->font.scale;
	usize codepoint_count = str8_codepoint_count(FONT_CHARSET);	
	atlas->atlas_width = size;
	atlas->atlas_height = size;
	atlas->atlas_data = (u8 *) malloc(size * size); // atlas data is allocated in the global allocator
	atlas->glyphs = glyph_map_new(allocator, codepoint_count);
	atlas->current_x = 1;
	atlas->current_y = 1;
	atlas->row_height = 0;
	Texture_Data tex_data = {
		.data = atlas->atlas_data,
		.width = atlas->atlas_width,
		.height = atlas->atlas_height,
		.channels = 1,
	};
	atlas->tex_id = gfx_texture_upload(tex_data, TextureKind_GreyScale);
	atlas->dirty  = false;
	
	return true;
}

internal bool
font_atlas_add_glyph(Font_Atlas *atlas, rune codepoint)
{
	Assert(atlas);
	
	{ // check if the glyph is already in the atlas
		Glyph_Info info;
		bool contains_glyph = glyph_map_get(atlas->glyphs, codepoint, &info);
		if (contains_glyph) {
			return true;
		}
	}
	
	stbtt_fontinfo *font_info = &atlas->font.font_info;
	i32 glyph_index = stbtt_FindGlyphIndex(font_info, (i32) codepoint);
	if (glyph_index == 0 && codepoint != ' ') {
		LogError("Codepoint %d missing in Font", codepoint);
		return false;
	}
	
	i32 x0, y0, x1, y1;
	stbtt_GetGlyphBitmapBox(font_info, glyph_index, atlas->font.scale, atlas->font.scale, &x0, &y0, &x1, &y1);
	i32 glyph_width = x1 - x0;
	i32 glyph_height = y1 - y0;
	
	if (glyph_width <= 0 || glyph_height <= 0) {
		i32 advance_width;
		stbtt_GetGlyphHMetrics(font_info, glyph_index, &advance_width, NULL);
		
		Glyph_Info glyph = {
			.codepoint = codepoint,
			.x0 = 0, .y0 = 0, .x1 = 0, .y1 = 0,
			.xoff = 0, .yoff = 0,
			.xadvance = (f32)advance_width * atlas->font.scale,
		};
		return glyph_map_insert(atlas->glyphs, codepoint, glyph);
	}
	
	if (atlas->current_x + glyph_width + 1 > atlas->atlas_width) {
		atlas->current_x = 1;
		atlas->current_y += atlas->row_height + 1;
		atlas->row_height = 0;
	}
	
	if (atlas->current_y + glyph_height + 1 > atlas->atlas_height) {
		if (!font_atlas_expand(atlas)) {
			LogError("Failed to expand font atlas");
			return false;
		}
	}
	
	i32 glyph_x = atlas->current_x;
	i32 glyph_y = atlas->current_y;
	
	i32 bitmap_width, bitmap_height;
	u8 *glyph_bitmap = stbtt_GetGlyphBitmap(font_info,
		atlas->font.scale, atlas->font.scale,
		glyph_index, &bitmap_width, &bitmap_height, NULL, NULL);
	
	if (bitmap_width != glyph_width || bitmap_height != glyph_height) {
		LogWarn("Bitmap dimensions (%d x %d) don't match calculated dimensions (%d x %d) for glyph %c", bitmap_width, bitmap_height, glyph_width, glyph_height, (char)codepoint);
		glyph_width = bitmap_width;
		glyph_height = bitmap_height;
	}
	
	// Copy glyph bitmap to atlas
	if (glyph_bitmap != NULL && glyph_width > 0 && glyph_height > 0) {
		for (i32 row = 0; row < glyph_height; row++) {
			i32 atlas_y = glyph_y + row;
			if (atlas_y >= atlas->atlas_height) break;
			
			for (i32 col = 0; col < glyph_width; col++) {
				i32 atlas_x = glyph_x + col;
				if (atlas_x >= atlas->atlas_width) break;
				
				i32 atlas_idx = atlas_y * atlas->atlas_width + atlas_x;
				i32 glyph_idx = row * glyph_width + col;
				
				if (atlas_idx < (atlas->atlas_width * atlas->atlas_height) && glyph_idx >= 0) {
					atlas->atlas_data[atlas_idx] = glyph_bitmap[glyph_idx];
				}
			}
		}
	}
	
	if (glyph_bitmap != NULL) {
		stbtt_FreeBitmap(glyph_bitmap, NULL);
	}
	
	i32 advance_width;
	stbtt_GetGlyphHMetrics(font_info, glyph_index, &advance_width, NULL);
	
	Glyph_Info glyph = {
		.codepoint = codepoint,
		.x0 = glyph_x,
		.y0 = glyph_y,
		.x1 = glyph_x + glyph_width,
		.y1 = glyph_y + glyph_height,
		.xoff = (f32)x0,
		.yoff = (f32)y0,
		.xadvance = (f32)advance_width * atlas->font.scale,
	};
	
	atlas->current_x += glyph_width + 1;
	atlas->row_height = Max(atlas->row_height, glyph_height);
	atlas->dirty = true;
	
	return glyph_map_insert(atlas->glyphs, codepoint, glyph);
}

internal bool
font_atlas_expand(Font_Atlas *atlas)
{
	i32 new_width = atlas->atlas_width * 2;
	i32 new_height = atlas->atlas_height * 2;
	
	if (new_width > 4096 || new_height > 4096) {
		LogError("Font atlas size limit reached");
		return false;
	}
	
	u8 *new_data = (u8 *)malloc(new_width * new_height);
	MemZero(new_data, new_width * new_height);
	
	// Copy old data to new atlas
	for (i32 row = 0; row < atlas->atlas_height; row++) {
		i32 old_start = row * atlas->atlas_width;
		i32 new_start = row * new_width;
		MemMove(new_data + new_start, atlas->atlas_data + old_start, atlas->atlas_width);
	}
	
	free(atlas->atlas_data);
	atlas->atlas_data = new_data;
	atlas->atlas_width = new_width;
	atlas->atlas_height = new_height;
	
	gfx_texture_unload(atlas->tex_id);
	
	Texture_Data tex_data = {
		.data = atlas->atlas_data,
		.width = atlas->atlas_width,
		.height = atlas->atlas_height,
		.channels = 1,
	};
	
	atlas->tex_id = gfx_texture_upload(tex_data, TextureKind_GreyScale);
	atlas->dirty = false;
	
	LogInfo("Font atlas expanded to %dx%d", new_width, new_height);
	return true;
}

internal void
font_atlas_add_glyphs_from_string(Font_Atlas *atlas, String8 string)
{
	Assert(atlas);
	
	str8_foreach(string, itr, i) {
		rune codepoint = itr.codepoint;
		font_atlas_add_glyph(atlas, codepoint);
	}
}

internal void
font_atlas_update(Font_Atlas *atlas)
{
	if (!atlas->dirty) {
		return;
	}
	
	gfx_texture_update(atlas->tex_id, atlas->atlas_width, atlas->atlas_height, 1, atlas->atlas_data);
	atlas->dirty = false;
}

internal void
font_atlas_delete(Font_Atlas *atlas)
{
	Assert(atlas);
	
	gfx_texture_unload(atlas->tex_id);
	free(atlas->atlas_data);
	MemZero(atlas, sizeof(Font_Atlas));
}

internal bool
font_atlas_get_glyph(Font_Atlas *atlas, rune codepoint, Glyph_Info *out_glyph)
{
	Assert(atlas);
	return glyph_map_get(atlas->glyphs, codepoint, out_glyph);
}

internal bool
font_atlas_resize_glyphs(Arena *temp_arena, Font_Atlas *atlas, f32 new_font_height)
{
	Assert(atlas);

	f32 old_scale = atlas->font.scale;
	f32 new_scale = stbtt_ScaleForPixelHeight(&atlas->font.font_info, new_font_height);

	if (fabs(new_scale - old_scale) < 0.001f) {
		return true;
	}

	atlas->font.scale = new_scale;

	i32 ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&atlas->font.font_info, &ascent, &descent, &line_gap);

	atlas->font.ascent   = (f32)ascent   * atlas->font.scale;
	atlas->font.descent  = (f32)descent  * atlas->font.scale;
	atlas->font.line_gap = (f32)line_gap * atlas->font.scale;

	glyph_map *map = atlas->glyphs;
	usize glyph_count = map->size;

	rune *glyphs_to_regenerate = arena_push_array(temp_arena, rune, glyph_count);
	usize regen_count = 0;

	for (usize i = 0; i < map->capacity; i++) {
		if (!map->entries[i].occupied) continue;
		glyphs_to_regenerate[regen_count++] = map->entries[i].key;
	}

	usize total_pixels = atlas->atlas_width * atlas->atlas_height;
	MemZero(atlas->atlas_data, total_pixels);

	glyph_map_clear(map);

	atlas->current_x = 1;
	atlas->current_y = 1;
	atlas->row_height = 0;

	for (usize i = 0; i < regen_count; i++) {
		rune cp = glyphs_to_regenerate[i];

		if (!font_atlas_add_glyph(atlas, cp)) {
			u8 buffer[4];
			String8 utf8 = str8_encode_rune(cp, buffer);
			LogWarn("Failed to regenerate glyph " STR_FMT " after resize",
					 str8_fmt(utf8));
		}
	}

	font_atlas_update(atlas);
	return true;
}

internal f32
font_atlas_height(Font_Atlas *atlas)
{
	return atlas->font.ascent - atlas->font.descent + atlas->font.line_gap;
}

