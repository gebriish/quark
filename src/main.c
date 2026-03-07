#include "base.h"
#include "gfx.h"
#include "draw.h"
#include "glyph_cache.h"
#include "buffer.h"
#include "editor.h"

#include "base.c"
#include "gfx.c"
#include "draw.c"
#include "glyph_cache.c"
#include "buffer.c"
#include "editor.c"

typedef u32 Cli_Parse_Mode;
enum {
	Cli_Path = 0,
	Cli_Memory_Limit,
};

typedef struct {
	Cli_Parse_Mode mode;
	String8_List paths;
} Command_Line_Args;

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

		if (pen_y > screen_rect.to.y)
			break;

		vec2 pos  = { pen_x, pen_y };
		vec2 size = { cell_w, cell_h };

		bool visible =
			!(pen_y + cell_h < screen_rect.from.y ||
			pen_y > screen_rect.to.y);

		if (itr.is_on_cursor) {
			cursor_target = pos;
			cursor_cp     = c;
			cursor_found  = true;
		}

		if (c == '\n') {
			pen_x = 10.0;
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
			push_glyph(cache, c, pos, size, 0x131313ff);
		}

		pen_x += cell_w;
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
			 0x131313ff,
			 WHITE_TEXTURE);


	if (cursor_found && !is_space(cursor_cp)) {
		push_glyph(cache, cursor_cp, cursor_visual, (vec2){ cell_w, cell_h }, 0x99856aff);
	}

	vec2 quad_pos =  { screen_rect.from.x, screen_rect.to.y - cell_h };
	vec2 quad_size = { screen_rect.to.x - screen_rect.from.x, cell_h };
	draw_quad(quad_pos, quad_size, 0x131313ff);

	internal String8 mode_string[Mode_Count] = {
		[Mode_Normal] = S("NORMAL"),
		[Mode_Insert] = S("INSERT"),
		[Mode_Visual] = S("VISUAL"),
		[Mode_CLI]    = S("CMD_LN"),
	};
	
	draw_string_aligned(
		str8_tprintf(scratch, STR " ", s_fmt(buf->name)),
		quad_pos, quad_size, 0x99856aff, 4,
		(Box_Alignment) {AlignH_Right, AlignV_Center}, cache
	);
	draw_string_aligned(
		str8_tprintf(scratch, " -- " STR " --" , mode_string[Mode_Normal]),
		quad_pos, quad_size, 0x99856aff, 4,
		(Box_Alignment) {AlignH_Left, AlignV_Center}, cache
	);

}


int main(int argc, const char **argv)
{
	Allocator alloc       = heap_allocator();
	Allocator frame_alloc = arena_allocator(Mb(32));

	Command_Line_Args cli_args = cli_parse(argc, argv, frame_alloc);

	Editor_Context ctx = editor_context(alloc, frame_alloc);
	GFX_Context gfx = gfx_make(S("quark"), 1000, 625, alloc, frame_alloc);

	String8 ttf_data = os_data_from_path(S("./res/JetBrainsMono-Regular.ttf"), alloc);

	Glyph_Cache glyph_cache = {0};
	Glyph_Table_Params params = {
		.hash_count     = 1024,
		.entry_count    = 1024,
		.reserved_tiles = 0,
		.tiles_per_row  = 0,
	};

	glyph_cache_make(&glyph_cache,
				  ttf_data.str,
				  50,
				  512,
				  512,
				  25,
				  50,
				  params,
				  alloc,
				  frame_alloc);

	for (usize i = 0; i < cli_args.paths.len; ++i) {
		String8 path = dyn_arr_index(&cli_args.paths, String8, i);

		editor_push_cmd(&ctx, (Editor_Cmd){
			.type = Cmd_Buffer_Open,
			.buffer_open = { .name = path }
		});
	}

	if (!ctx.active_buffer) {
		editor_push_cmd(&ctx, (Editor_Cmd){
			.type = Cmd_Buffer_Open,
			.buffer_open = { .name = S("untitled") }
		});
	}


	editor_push_cmd(&ctx, (Editor_Cmd){
		.type = Cmd_Mode_Change,
		.mode = { .to = Mode_Insert }
	});

	for (;;) {
		mem_free_all(frame_alloc);
		if (!gfx_window_open()) break;

		Frame_Input input = gfx_frame_begin(0x99856aff);

		if (input.text.len) {
			editor_push_cmd(&ctx, (Editor_Cmd){
				.type = Cmd_Insert_Text,
				.text_insert = { .text = input.text }
			});
		}

		u32 flags = input.special_key_presses;

		if (MaskCheck(flags, Pressed_Enter)) {
			usize indent = buffer_current_indent_depth(ctx.active_buffer, ctx.tab_width);
			u8 *buf = alloc_array_nz(frame_alloc, u8, indent, NULL);
			memset(buf, '\t', indent);
			String8 indent_string = str8(buf, indent);

			String8 s = str8_concat(S("\n"), indent_string, frame_alloc);

			editor_push_cmd(&ctx, (Editor_Cmd){
				.type = Cmd_Insert_Text,
				.text_insert = { .text = s }
			});
		}
		else if (MaskCheck(flags, Pressed_Backspace)) {
			editor_push_cmd(&ctx, (Editor_Cmd){
				.type = Cmd_Delete_Text,
				.text_delete = { .amount = 1, .move = true }
			});
		}
		else if (MaskCheck(flags, Pressed_Delete)) {
			editor_push_cmd(&ctx, (Editor_Cmd){
				.type = Cmd_Delete_Text,
				.text_delete = { .amount = 1, .move = false }
			});
		}
		else if (MaskCheck(flags, Pressed_Move_Left)) {
			editor_push_cmd(&ctx, (Editor_Cmd){
				.type = Cmd_Cursor_Move,
				.cursor = { .dx = -1, .dy = 0 }
			});
		}
		else if (MaskCheck(flags, Pressed_Move_Right)) {
			editor_push_cmd(&ctx, (Editor_Cmd){
				.type = Cmd_Cursor_Move,
				.cursor = { .dx = 1, .dy = 0 }
			});
		}
		else if (MaskCheck(flags, Pressed_Move_Up)) {
			editor_push_cmd(&ctx, (Editor_Cmd){
				.type = Cmd_Cursor_Move,
				.cursor = { .dx = 0, .dy = -1 }
			});
		}
		else if (MaskCheck(flags, Pressed_Move_Down)) {
			editor_push_cmd(&ctx, (Editor_Cmd){
				.type = Cmd_Cursor_Move,
				.cursor = { .dx = 0, .dy = 1 }
			});
		}

		local_persist f32 scroll = -10.0;
		if (scroll >= -10.0)
			scroll -= input.scroll_y * 90;
		else
			scroll = -10.0;
		editor_render(ctx.active_buffer, frame_alloc, &glyph_cache, scroll );

		gfx_frame_end();
	}

	return 0;
}
