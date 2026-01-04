#define DEBUG_BUILD 1

// こんにちは

#include "base/base_inc.h"
#include "gfx/gfx_inc.h"
#include "os/os_inc.h"
#include "quark/quark_inc.h"

#include "base/base_inc.c"
#include "gfx/gfx_inc.c"
#include "os/os_inc.c"
#include "quark/quark_inc.c"

global Quark_Ctx  quark_ctx;
global GFX_State  gfx_state;

internal f64 
get_delta_time(OS_Time_Stamp *last_frame_time) {
	OS_Time_Stamp frame_start = os_time_now();
	f64 dt = os_time_diff(*last_frame_time, frame_start).seconds;
	*last_frame_time = frame_start;
	return dt;
}

int main(int argc, char **argv)
{
	gfx_init(&gfx_state);

	quark_ctx = quark_new();
	quark_set_context(&quark_ctx);

	string8_list args = str8_list_from_cstring_array(quark_ctx.arena, argc, argv);

	String8 file_data = S("");

	if (args.len == 2) {
		file_data = os_data_from_file_path(quark_ctx.frame_arena, args.array[1]);
	}

	QBuffer *buf = quark_buffer_new(quark_ctx.buffer_arena, file_data);

	OS_Time_Stamp last_time = os_time_now();
	for(;;) 
	{
		arena_reset(quark_ctx.frame_arena);

		if (!gfx_window_open()) break;
		f64 dt = get_delta_time(&last_time);
		vec2_i32 window_size = gfx_window_size();


		gfx_begin_frame(0x171717ff);

		Font_Atlas *atlas = &gfx_state.font_atlas;

		f32 x = 0, y = 0;
	
		const f32 atlas_w = (f32)atlas->atlas_width;
		const f32 atlas_h = (f32)atlas->atlas_height;
		const f32 line_height = font_atlas_height(atlas);

		Glyph_Info probe;
		font_atlas_get_glyph(atlas, 'A', &probe);
		f32 mono_advance = probe.xadvance;

		QBuffer_Itr itr = {0};
		for (itr = quark_buffer_itr(buf, NULL); itr.ptr != NULL; itr = quark_buffer_itr(buf, &itr))
		{
			if (y > window_size.y) { break; }

			rune c = itr.codepoint;

			if (c == '\n')
			{
				x = 0;
				y += line_height;
				continue;
			}
			if (c == ' ')
			{
				x += mono_advance;
				continue;
			}
			if (c == '\t')
			{
				x += mono_advance * 4.0f;
				continue;
			}

			Glyph_Info info;
			bool missing = false;
			if (!font_atlas_get_glyph(atlas, c, &info))
			{
				missing = true;
				font_atlas_get_glyph(atlas, '?', &info);
			}

			f32 glyph_x = x + info.xoff;
			f32 glyph_y = y + atlas->font.ascent + info.yoff;

			f32 gw = (f32)(info.x1 - info.x0);
			f32 gh = (f32)(info.y1 - info.y0);

			if (gw > 0 && gh > 0)
			{
				gfx_push_rect(&(Rect_Params){
					.position = {floorf(glyph_x), floorf(glyph_y)},
					.size = {gw, gh},
					.color = missing ? 0xcc241dff : 0x928374ff,
					.uv = {
						(f32)info.x0 / atlas_w,
						(f32)info.y0 / atlas_h,
						(f32)info.x1 / atlas_w,
						(f32)info.y1 / atlas_h},
					.tex_id = atlas->tex_id,
				});
			}

			x += mono_advance;
		}

		gfx_end_frame();
	}

	quark_delete();
	gfx_deinit();
}
