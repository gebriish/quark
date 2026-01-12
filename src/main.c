#define DEBUG_BUILD 1

#include "base/base_inc.h"
#include "gfx/gfx_inc.h"
#include "os/os_inc.h"
#include "quark/quark_inc.h"

#include "base/base_inc.c"
#include "gfx/gfx_inc.c"
#include "os/os_inc.c"
#include "quark/quark_inc.c"


internal f64
get_delta_time(OS_Time_Stamp *last_frame_time)
{
	OS_Time_Stamp frame_start = os_time_now();
	f64 dt = os_time_diff(*last_frame_time, frame_start).seconds;
	*last_frame_time = frame_start;
	return dt;
}

int main(int argc, char **argv)
{
	Quark_Ctx quark_ctx = {0};
	GFX_State gfx_state = {0};

	gfx_init(&gfx_state);

	quark_ctx = quark_new();
	quark_set_context(&quark_ctx);

	string8_list args = str8_list_from_cstring_array(quark_ctx.arena, argc, argv);

	if (args.len == 1) {
		Quark_Cmd cmd = {
			.kind = Cmd_Kind_New_Buffer,
			.buffer_new = {
				.name = S(""),
				.data_buffer = S(""),
			}
		};
		quark_exec_cmd(
			cmd
		);
	}
	else {
		Quark_Cmd cmd = {
			.kind = Cmd_Kind_Open_File,
			.open_file = {
				.path = args.array[1],
			}
		};
		quark_exec_cmd(
			cmd
		);
	}

	OS_Time_Stamp last_time = os_time_now();
	QBuffer *buf = quark_ctx.active_buffer;
	for (;;)
	{
		if (!gfx_window_open()) break;

		f64 dt = get_delta_time(&last_time);
		arena_reset(quark_ctx.frame_arena);

		gfx_begin_frame(0x171717ff);

		vec2_i32 window_size = gfx_window_size();
		Input_Data input_data = gfx_input_data();

		switch (input_data.codepoint) {
			case ASCII_NUL: break;
			case ASCII_BS:
			case ASCII_DEL: {
				usize idx = buf->gap_index;
				quark_buffer_delete(buf, 1, idx, input_data.codepoint == ASCII_BS );
			} break;

			default: {
				u8 encode_buf[4];
				String8 encoded = utf8_encode(input_data.codepoint, encode_buf);
				usize idx = buf->gap_index;
				quark_buffer_insert(buf, encoded, idx);
			} break;
		}

		Font_Atlas *atlas = &gfx_state.font_atlas;

		const f32 atlas_w = (f32)atlas->atlas_width;
		const f32 atlas_h = (f32)atlas->atlas_height;
		const f32 line_height = font_atlas_height(atlas);

		Glyph_Info probe;
		font_atlas_get_glyph(atlas, 'A', &probe);
		const f32 mono_advance = probe.xadvance;

		f32 x = 5.0f;
		f32 y = 5.0f;

		bool cursor_seen = false;
		f32 cursor_x = 0.0f;
		f32 cursor_y = 0.0f;

		rune char_over_cursor = Utf8_Invalid;
		QBuffer_Itr itr = {0};
		for (itr = quark_buffer_itr(buf, NULL); itr.ptr != NULL; itr = quark_buffer_itr(buf, &itr)) 
		{
			if (y > window_size.y) break;

			if (itr.on_cursor)
			{
				cursor_x = x;
				cursor_y = y;
				cursor_seen = true;
				char_over_cursor = itr.codepoint;
			}

			rune c = itr.codepoint;

			if (c == '\n')
			{
				x = 5;
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

			if (!itr.on_cursor) {
				Glyph_Info info;
				bool missing = false;
				if (!font_atlas_get_glyph(atlas, c, &info) || !info.resident)
				{
					missing = true;
					font_atlas_get_glyph(atlas, '?', &info);
				}

				f32 glyph_x = x + info.xoff;
				f32 glyph_y = y + atlas->font.ascent + info.yoff;

				f32 gw = (f32)(info.x1 - info.x0);
				f32 gh = (f32)(info.y1 - info.y0);

				gfx_push_rect((Rect_Params){
					.position = { floorf(glyph_x), floorf(glyph_y) },
					.size     = { gw, gh },
					.color    = missing ? 0xcc241dff : 0x928374ff,
					.uv = {
						(f32)info.x0 / atlas_w,
						(f32)info.y0 / atlas_h,
						(f32)info.x1 / atlas_w,
						(f32)info.y1 / atlas_h,
					},
					.tex_id = atlas->tex_id,
				});
			}

			x += mono_advance;
		}

		if (!cursor_seen)
		{
			cursor_x = x;
			cursor_y = y;
		}

		static f32 cx = 0.0f;
		static f32 cy = 0.0f;

		cx = smooth_damp(cx, cursor_x, 0.03f, dt);
		cy = smooth_damp(cy, cursor_y, 0.03f, dt);

		gfx_push_rect_rounded((Rect_Params){
			.position = { cx, cy },
			.size     = { mono_advance, line_height },
			.color    = 0xb8bb26ff,
			.radii    = rect_radius(mono_advance * 0.5f),
		});

		if (char_over_cursor != Utf8_Invalid && !rune_is_space(char_over_cursor)) {
			Glyph_Info info;
			bool missing = false;
			if (!font_atlas_get_glyph(atlas, char_over_cursor, &info))
			{
				missing = true;
				font_atlas_get_glyph(atlas, '?', &info);
			}

			f32 glyph_x = cursor_x + info.xoff;
			f32 glyph_y = cursor_y + atlas->font.ascent + info.yoff;

			f32 gw = (f32)(info.x1 - info.x0);
			f32 gh = (f32)(info.y1 - info.y0);

			gfx_push_rect((Rect_Params){
				.position = { floorf(glyph_x), floorf(glyph_y) },
				.size     = { gw, gh },
				.color    = missing ? 0xcc241dff : 0x171717ff,
				.uv = {
					(f32)info.x0 / atlas_w,
					(f32)info.y0 / atlas_h,
					(f32)info.x1 / atlas_w,
					(f32)info.y1 / atlas_h,
				},
				.tex_id = atlas->tex_id,
			});
		}

		gfx_end_frame();
	}

	quark_delete();
	gfx_deinit();
}
