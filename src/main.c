#include "base.h"
#include "gfx.h"
#include "draw.h"
#include "glyph_cache.h"
#include "buffer.h"

#include "base.c"
#include "gfx.c"
#include "draw.c"
#include "glyph_cache.c"
#include "buffer.c"

typedef u32 Cli_Parse_Mode;
enum {
	Cli_Path = 0,
	Cli_Memory_Limit,
};

typedef struct {
	Cli_Parse_Mode mode;
	String8_List paths;
} Command_Line_Args;

typedef struct {
} Editor_State;


internal Command_Line_Args
cli_parse(int argc, const char **argv, Allocator frame_alloc)
{
	Command_Line_Args cli_args = {0};
	cli_args.paths = dynamic_array(frame_alloc, String8, 8);

	String8_List args = str8_make_list(argv, (usize)argc, frame_alloc);

	for (usize i = 1; i < args.len; ++i) {
		String8 arg = dyn_arr_index(&args, String8, i);

		if (str8_equal(arg, S("-m"))) {
			cli_args.mode = Cli_Memory_Limit;
			continue;
		}

		switch (cli_args.mode) {
		case Cli_Path:
			dyn_arr_append(&cli_args.paths, String8, arg);
			break;

		case Cli_Memory_Limit:
			cli_args.mode = Cli_Path;
			break;
		}
	}

	return cli_args;
}


internal Q_Buffer *
editor_handle_input(Q_Buffer *buf, Frame_Input input)
{
	if (input.text.len) {
		return buffer_insert(buf, input.text);
	}

	u32 flags = input.special_key_presses;

	if (MaskCheck(flags, Pressed_Enter)) {
		rune c = '\n';
		String8 s = {0};
		s.str = cast(u8 *) &c;
		s.len = 1;
		return buffer_insert(buf, s);
	}
	else if (MaskCheck(flags, Pressed_Backspace)) {
		if (buf->gap_pos <= 0) return buf;
		buffer_move_left(buf);
		buffer_erase(buf);
	}
	else if (MaskCheck(flags, Pressed_Delete)) {
		buffer_erase(buf);
	}
	else if (MaskCheck(flags, Pressed_Move_Left))  buffer_move_left(buf);
	else if (MaskCheck(flags, Pressed_Move_Right)) buffer_move_right(buf);
	else if (MaskCheck(flags, Pressed_Move_Up))    buffer_move_up(buf);
	else if (MaskCheck(flags, Pressed_Move_Down))  buffer_move_down(buf);

	return buf;
}

internal void
push_glyph(Glyph_Cache *cache, rune c, vec2 pos, vec2 size, u32 color)
{
	Glyph_State state = glyph_get(cache, c);
	if (!state.filled) return;

	Glyph_Cache_Point p = glyph_unpack_point(state.gpu_index);

	f32 atlas_x = (f32)(p.x * cache->tile_width);
	f32 atlas_y = (f32)(p.y * cache->tile_height);

	vec2 uv0 = {
		atlas_x / (f32)cache->atlas_width,
		atlas_y / (f32)cache->atlas_height
	};

	vec2 uv1 = {
		(atlas_x + cache->tile_width)  / (f32)cache->atlas_width,
		(atlas_y + cache->tile_height) / (f32)cache->atlas_height
	};

	gfx_push_rect(
		pos,
		size,
		color,
		cache->texture,
		(Rect){ uv0, uv1 },
		(Rect){ {0.5f, 0.5f}, {0.5f, 0.5f} }
	);
}

internal f32 
smooth_damp(f32 current, f32 target, f32 time, f32 dt)
{
	if (dt <= 0 || time <= 0) return target;

	f32 rate = 2.0f / time;
	f32 x = rate * dt;

	f32 factor = 0;
	if (x < 0.0001f) {
		factor = x * (1.0f - x*0.5f + x*x/6.0f - x*x*x/24.0f);
	} else {
		factor = 1.0f - expf(-x);
	}

	return Lerp(current, target, factor);
}

internal void
editor_render(Q_Buffer *buf, Allocator scratch, Glyph_Cache *cache, f32 y_level)
{
    f32 pen_x = 10.0f;
    f32 pen_y = -y_level;

    f32 cell_w = (f32)cache->tile_width;
    f32 cell_h = (f32)cache->tile_height;

    Rect screen_rect = gfx_get_clip_rect();

    vec2 cursor_target = { pen_x, pen_y };
    bool cursor_found = false;
    rune cursor_cp = 0;

    Q_Iterator itr = {0};

	while (buffer_iter(buf, &itr)) {

		rune c = itr.codepoint;

		vec2 pos = { pen_x, pen_y };
		vec2 size = { cell_w, cell_h };

		bool visible =
			!(pen_y + cell_h < screen_rect.from.y ||
			pen_y > screen_rect.to.y);

		if (itr.is_on_cursor) {
			cursor_target = pos;
			cursor_cp = c;
			cursor_found = true;
		}

		if (c == '\n') {
			pen_x = 10.0f;
			pen_y += cell_h;
			continue;
		}

		if (c == ' ') {
			pen_x += cell_w;
			continue;
		}

		if (c == '\t') {
			pen_x += cell_w * 4.0f;
			continue;
		}

		if (visible) {
			push_glyph(cache, c, pos, size, 0x99856aff);
		}

		pen_x += cell_w;

		if (pen_y > screen_rect.to.y)
			break;
	}

    if (!cursor_found) {
        cursor_target = (vec2){ pen_x, pen_y };
    }

    static vec2 cursor_visual = {0};
    static bool initialized = false;

    if (!initialized) {
        cursor_visual = cursor_target;
        initialized = true;
    }

    f32 dt = cast(f32) gfx_delta_time();
    f32 smooth_time = 0.04f;

    cursor_visual.x = smooth_damp(cursor_visual.x, cursor_target.x, smooth_time, dt);
    cursor_visual.y = smooth_damp(cursor_visual.y, cursor_target.y, smooth_time, dt);

    draw_cursor(cursor_visual,
                (vec2){ cell_w, cell_h },
                0x00ff00ff,
                WHITE_TEXTURE);


    if (cursor_found &&
        cursor_cp != '\n' &&
        cursor_cp != ' '  &&
        cursor_cp != '\t') {
        push_glyph(cache,
                   cursor_cp,
                   cursor_visual,
                   (vec2){ cell_w, cell_h },
                   0x000000ff);
    }
}


int main(int argc, const char **argv)
{
	Allocator alloc       = heap_allocator();
	Allocator frame_alloc = arena_allocator(Mb(32));

	Command_Line_Args cli_args = cli_parse(argc, argv, frame_alloc);

	GFX_Context gfx = gfx_new(
		S("quark"), 1000, 625, alloc, frame_alloc
	);

	String8 ttf_data =
		os_data_from_path(S("./res/JetBrainsMono-Regular.ttf"), alloc);

	Glyph_Cache glyph_cache = {0};
	Glyph_Table_Params params = {
		.hash_count     = 1024,
		.entry_count    = 1024,
		.reserved_tiles = 0,
		.tiles_per_row  = 0,
	};

	glyph_cache_make(
		&glyph_cache,
		ttf_data.str,
		20,
		512, 512,
		10, 20,
		params,
		alloc,
		frame_alloc
	);

	String8 file_data = S("");
	if (cli_args.paths.len) {
		file_data = os_data_from_path(
			dyn_arr_index(&cli_args.paths, String8, 0),
			frame_alloc
		);
	}

	Q_Buffer *buf = buffer_new(file_data, NULL, alloc);

	mem_free_all(frame_alloc);

	for (;;) {
		if (!gfx_window_open()) break;

		Frame_Input input = gfx_frame_begin(0x101010ff);


		buf = editor_handle_input(buf, input);
		editor_render(buf, frame_alloc, &glyph_cache, 0);

		gfx_frame_end();
		mem_free_all(frame_alloc);
	}

	return 0;
}
