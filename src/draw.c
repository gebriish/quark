#include "draw.h"

////////////////////////////////////////

internal vec2 _measure_string(String8 string, int tab_width, Glyph_Cache *cache);

////////////////////////////////////////

internal void
draw_cursor(vec2 pos, vec2 size, color8_t color, u32 texture)
{
	f32 diameter = size.x;
	f32 radius   = diameter * 0.5f;

	if (size.y <= diameter) {
		gfx_push_rect(
			pos,
			(vec2){ diameter, diameter },
			color,
			texture,
			UV_FULL,
			UV_FULL
		);
		return;
	}

	gfx_push_rect(
		pos,
		(vec2){ diameter, radius },
		color,
		texture,
		UV_FULL,
		(Rect){{0,0},{1,0.5f}}
	);

	gfx_push_rect(
		(vec2){ pos.x, pos.y + radius },
		(vec2){ diameter, size.y - diameter },
		color,
		texture,
		UV_FULL,
		CIRC_CENTER
	);

	gfx_push_rect(
		(vec2){ pos.x, pos.y + size.y - radius },
		(vec2){ diameter, radius },
		color,
		texture,
		UV_FULL,
		(Rect){{0,0.5f},{1,1}}
	);
}

internal void
draw_quad(vec2 pos, vec2 size, color8_t color)
{
	gfx_push_rect(pos, size, color, WHITE_TEXTURE, UV_FULL, CIRC_CENTER);
}

internal void
draw_quad_textured(vec2 pos, vec2 size, color8_t color, u32 texture)
{
	gfx_push_rect(pos, size, color, texture, UV_FULL, CIRC_CENTER);
}

internal void
draw_string(String8 string, vec2 position, color8_t color, int tab_width, Glyph_Cache *cache)
{
    f32 pen_x = position.x;
    f32 pen_y = position.y;

    f32 cell_w = (f32)cache->tile_width;
    f32 cell_h = (f32)cache->tile_height;

    Str_Iterator itr = {0};
    while (str8_iter(string, &itr))
    {
        rune c = itr.codepoint;

        if (c == '\n') {
            pen_x = position.x;
            pen_y += cell_h;
            continue;
        } else if (c == ' ') {
			pen_x += cell_w;
			continue;
		} else if (c == '\t') {
            pen_x += cell_w * tab_width;
            continue;
        }

        Glyph_State state = glyph_get(cache, c);
        if (!state.filled)
            continue;

        Glyph_Cache_Point p = glyph_unpack_point(state.gpu_index);

        f32 atlas_x = (f32)(p.x * cache->tile_width);
        f32 atlas_y = (f32)(p.y * cache->tile_height);

        vec2 pos  = { pen_x, pen_y };
        vec2 size = { cell_w, cell_h };

        vec2 uv0 = {
            atlas_x / (f32)cache->atlas_width,
            atlas_y / (f32)cache->atlas_height
        };

        vec2 uv1 = {
            (atlas_x + cell_w) / (f32)cache->atlas_width,
            (atlas_y + cell_h) / (f32)cache->atlas_height
        };

        gfx_push_rect(
            pos,
            size,
            color,
            cache->texture,
            (Rect){ uv0, uv1 },
            (Rect){ {0.5f, 0.5f}, {0.5f, 0.5f} }
        );

        pen_x += cell_w;
    }
}

internal void
draw_string_aligned(String8 string, vec2 position, vec2 box_size, color8_t color, int tab_width, Box_Alignment alignment, Glyph_Cache *cache)
{
    if (!string.len) return;

    vec2 text_size = _measure_string(string, tab_width, cache);

    vec2 offset = {0};

	switch (alignment.h) {
		case AlignH_Center:
			offset.x = (box_size.x - text_size.x) * 0.5;
			if (offset.x < 0) offset.x = 0;
		break;
		case AlignH_Right:
			offset.x = box_size.x - text_size.x;
			if (offset.x < 0) offset.x = 0;
		break;
	}

	switch (alignment.v) {
		case AlignV_Center:
			offset.y = (box_size.y - text_size.y) * 0.5;
			if (offset.y < 0) offset.y = 0;
		break;
		case AlignV_Bottom:
			offset.y = box_size.y - text_size.y;
			if (offset.y < 0) offset.y = 0;
		break;
	}

    vec2 final_pos = {
        position.x + offset.x,
        position.y + offset.y,
    };

    draw_string(string, final_pos, color, tab_width, cache);
}

////////////////////////////////////////

internal vec2
_measure_string(String8 string, int tab_width, Glyph_Cache *cache)
{
    f32 cell_w = (f32)cache->tile_width;
    f32 cell_h = (f32)cache->tile_height;

    f32 pen_x = 0;
    f32 max_x = 0;
    f32 total_h = cell_h;

    Str_Iterator itr = {0};
    while (str8_iter(string, &itr))
    {
        rune c = itr.codepoint;

        if (c == '\n') {
            if (pen_x > max_x) max_x = pen_x;
            pen_x = 0;
            total_h += cell_h;
        }
        else if (c == '\t')
            pen_x += cell_w * tab_width;
        else 
			pen_x += cell_w;
    }

    if (pen_x > max_x)
		max_x = pen_x;

    return (vec2){ max_x, total_h };
}
