#include "gfx_font.h"
#include "gfx_core.h"

internal bool
font_atlas_new(Arena *arena, u8 *font_data, i32 size, f32 font_height, Font_Atlas *atlas)
{
	Assert(atlas && font_data);
	
	if (!stbtt_InitFont(&atlas->font.font_info, font_data, 0)) {
		log_error("Failed to Initialize STB Font Info");
		return false;
	}
	atlas->font.scale = stbtt_ScaleForPixelHeight(&atlas->font.font_info, font_height);
	
	i32 ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&atlas->font.font_info, &ascent, &descent, &line_gap);
	
	atlas->font.ascent   = (f32)(ascent)   * atlas->font.scale;
	atlas->font.descent  = (f32)(descent)  * atlas->font.scale;
	atlas->font.line_gap = (f32)(line_gap) * atlas->font.scale;
	usize codepoint_count = str8_count(FONT_CHARSET);	
	atlas->atlas_width = size;
	atlas->atlas_height = size;
	atlas->atlas_data = arena_push_array(arena, u8, size * size);
	atlas->glyphs = glyph_map_make(arena, codepoint_count);
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
font_atlas_add_glyph(Font_Atlas *atlas, rune codepoint, bool ignore_chache)
{
	Assert(atlas);
	
	if (!ignore_chache) { // check if the glyph is already in the atlas
		Glyph_Info info;
		bool contains_glyph = glyph_map_get(&atlas->glyphs, codepoint, &info);
		if (contains_glyph) {
			return true;
		}
	}
	
	stbtt_fontinfo *font_info = &atlas->font.font_info;
	i32 glyph_index = stbtt_FindGlyphIndex(font_info, (i32) codepoint);
	if (glyph_index == 0 && codepoint != ' ') {
		log_error("Codepoint %d missing in font", codepoint);
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
			.resident = false,
		};

		return glyph_map_put(&atlas->glyphs, codepoint, glyph);
	}
	
	if (atlas->current_x + glyph_width + 1 > atlas->atlas_width) {
		atlas->current_x = 1;
		atlas->current_y += atlas->row_height + 1;
		atlas->row_height = 0;
	}
	
	if (atlas->current_y + glyph_height + 1 > atlas->atlas_height) {
		i32 advance_width;
		stbtt_GetGlyphHMetrics(font_info, glyph_index, &advance_width, NULL);

		Glyph_Info glyph = {
			.codepoint = codepoint,
			.x0 = 0, .y0 = 0, .x1 = 0, .y1 = 0,
			.xoff = (f32)x0,
			.yoff = (f32)y0,
			.xadvance = (f32)advance_width * atlas->font.scale,
			.resident = false,
		};

		glyph_map_put(&atlas->glyphs, codepoint, glyph);
		return false;
	}
	
	i32 glyph_x = atlas->current_x;
	i32 glyph_y = atlas->current_y;
	
	i32 bitmap_width, bitmap_height;
	u8 *glyph_bitmap = stbtt_GetGlyphBitmap(font_info,
		atlas->font.scale, atlas->font.scale,
		glyph_index, &bitmap_width, &bitmap_height, NULL, NULL);
	
	if (bitmap_width != glyph_width || bitmap_height != glyph_height) {
		log_warn("Bitmap dimensions (%d x %d) don't match calculated dimensions (%d x %d) for glyph %c", bitmap_width, bitmap_height, glyph_width, glyph_height, (char)codepoint);
		glyph_width = bitmap_width;
		glyph_height = bitmap_height;
	}
	
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
		.resident = true,
	};

	atlas->current_x += glyph_width + 1;
	atlas->row_height = Max(atlas->row_height, glyph_height);
	atlas->dirty = true;
	
	bool ok = glyph_map_put(&atlas->glyphs, codepoint, glyph);
	if (!ok) {
		log_error("foooo");
	}
	return ok;
}

internal bool
font_atlas_expand(Arena *arena, Font_Atlas *atlas)
{
  i32 old_width  = atlas->atlas_width;
  i32 old_height = atlas->atlas_height;

  i32 new_width  = old_width  * 2;
  i32 new_height = old_height * 2;

  if (new_width > 4096 || new_height > 4096) {
    log_error("Font atlas size limit reached");
		return false;
  }

  usize old_size = (usize)old_width * (usize)old_height;
  usize new_size = (usize)new_width * (usize)new_height;

  u8 *new_data = (u8 *)arena_realloc(
    arena,
    atlas->atlas_data,
    new_size,
    old_size
  );

  if (new_size > old_size) {
    MemZero(new_data + old_size, new_size - old_size);
  }

  atlas->atlas_data   = new_data;
  atlas->atlas_width  = new_width;
  atlas->atlas_height = new_height;

  gfx_texture_unload(atlas->tex_id);
  Texture_Data tex_data = {
    .data     = atlas->atlas_data,
    .width    = atlas->atlas_width,
    .height   = atlas->atlas_height,
    .channels = 1,
  };

  atlas->tex_id = gfx_texture_upload(tex_data, TextureKind_GreyScale);
  atlas->dirty  = false;

  log_trace("Font atlas expanded to %dx%d", new_width, new_height);
  return true;
}


internal void
font_atlas_add_glyphs_from_string(Font_Atlas *atlas, String8 string)
{
	Assert(atlas);

	usize consumed = 0;
	for (usize i=0; i<string.len; i += consumed) {
		rune codepoint = 0;
		UTF8_Error err = utf8_decode(string, i, &codepoint, &consumed);

		if (err) {
			log_error("utf8 decoding error code : %d", err);
			break;
		}

		font_atlas_add_glyph(atlas, codepoint, false);
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

internal bool
font_atlas_get_glyph(Font_Atlas *atlas, rune codepoint, Glyph_Info *out_glyph)
{
	Assert(atlas);
	return glyph_map_get(&atlas->glyphs, codepoint, out_glyph);
}

internal bool
font_atlas_resize_glyphs(Arena *scratch, Font_Atlas *atlas, f32 new_font_height)
{
	Assert(atlas);

	f32 new_scale = stbtt_ScaleForPixelHeight(&atlas->font.font_info, new_font_height);
	if (fabs(new_scale - atlas->font.scale) < 0.001f) {
		return true;
	}

	atlas->font.scale = new_scale;

	i32 ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&atlas->font.font_info, &ascent, &descent, &line_gap);

	atlas->font.ascent   = ascent   * new_scale;
	atlas->font.descent  = descent  * new_scale;
	atlas->font.line_gap = line_gap * new_scale;

	glyph_map *map = &atlas->glyphs;

	usize glyph_count = map->len;
	rune *glyphs = arena_push_array(scratch, rune, glyph_count);

	usize count = 0;
	for (usize i = 0; i < map->cap; i++) {
		if (!map->entries[i].used) continue;

		// mark all glyphs as non-resident up front
		map->entries[i].value.resident = false;
		glyphs[count++] = map->entries[i].key;
	}

	MemZero(atlas->atlas_data,
				 atlas->atlas_width * atlas->atlas_height);

	atlas->current_x = 1;
	atlas->current_y = 1;
	atlas->row_height = 0;

	for (usize i = 0; i < count; i++) {
		font_atlas_add_glyph(atlas, glyphs[i], true);
	}

	font_atlas_update(atlas);
	return true;
}

internal f32
font_atlas_height(Font_Atlas *atlas)
{
	return atlas->font.ascent - atlas->font.descent + atlas->font.line_gap;
}

