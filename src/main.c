#define DEBUG_BUILD 1

////////////////////////////////
// ~geb: .h Includes
#include "base/base_inc.h"
#include "gfx/gfx_inc.h"
#include "quark/quark_inc.h"
#include "os/os_inc.h"

////////////////////////////////
// ~geb: .c Includes
#include "base/base_inc.c"
#include "gfx/gfx_inc.c"
#include "quark/quark_inc.c"
#include "os/os_inc.c"

global Quark_Context quark_ctx;
global GFX_State     gfx_state;
global Font_Atlas    font_atlas;
global u8            font_tex_id;

int main(void)
{
	// ~geb: Layer initializations
	quark_new(&quark_ctx);

	gfx_init(&quark_ctx.allocator, &quark_ctx.temp_allocator, &gfx_state, quark_get_ogl_proc_addr);

	String8 file_data = os_read_file_data(
		&quark_ctx.allocator, 
		str8_lit("./res/fonts/JetBrainsMono-Regular.ttf")
	);
	font_atlas_new(&quark_ctx.allocator, file_data.raw, 512, 22, &font_atlas);
	font_atlas_add_glyphs_from_string(&quark_ctx.allocator, &font_atlas, FONT_CHARSET);
	font_tex_id = font_atlas.tex_id;

	Quark_Buffer *buff = q_buffer_new(&quark_ctx.allocator, &quark_ctx.buffer_manager, KB(10));

	OS_Time_Stamp last_time = os_time_now();

	vec2_f32 cursor_pos = {0};
	vec2_f32 target_cursor_pos = {0};

	while (quark_window_is_open(quark_ctx.window)) {
		font_atlas_update(&font_atlas);

		mem_free_all(&quark_ctx.temp_allocator);
		OS_Time_Stamp frame_start = os_time_now();
		f64 delta_time = os_time_diff(last_time, frame_start).seconds;
		last_time = frame_start;

		vec2_i32 window_size = quark_window_size(quark_ctx.window);

		Input_Data frame_input = quark_gather_input(quark_ctx.window, quark_ctx.state);
		Press_Flags s_flags = frame_input.special_press;

		gfx_begin_frame(0x131313FF);

		// Handle input
		if (quark_ctx.state == Quark_State_Insert) {
			if (s_flags & Press_Enter) {
				q_buffer_insert(buff, str8_lit("\n"));
			} else if (s_flags & Press_Backspace) {
				q_buffer_delete_rune(buff, 1, Cursor_Dir_Left);
			} else if (s_flags & Press_Delete) {
				q_buffer_delete_rune(buff, 1, Cursor_Dir_Right);
			} else if (s_flags & Press_Tab) {
				q_buffer_insert(buff, str8_lit("\t"));
			} else if (frame_input.codepoint) {
				u8 mem[4] = {0};
				String8 encoded_rune = str8_encode_rune(frame_input.codepoint, mem);
				q_buffer_insert(buff, encoded_rune);
			}
		}
		else if (quark_ctx.state == Quark_State_Normal) {
			if (frame_input.codepoint) {
				if (frame_input.codepoint == 'p') {
					f32 line_height = font_atlas_height(&font_atlas);
					if (line_height < 80) {
						font_atlas_resize_glyphs(&quark_ctx.temp_allocator, &font_atlas, line_height + 4);
					}
				}
				if (frame_input.codepoint == 'n') {
					f32 line_height = font_atlas_height(&font_atlas);
					if (line_height > 4) {
						font_atlas_resize_glyphs(&quark_ctx.temp_allocator, &font_atlas, line_height - 4);
					}
				}
			}
		}

		if (s_flags & Press_Left) {
			q_buffer_move_gap_by(buff, 1, Cursor_Dir_Left);
		} else if (s_flags & Press_Right) {
			q_buffer_move_gap_by(buff, 1, Cursor_Dir_Right);
		} else if (s_flags & Press_Down) {
			u32 into_line = runes_from(buff, '\n');
			u32 to_newline = runes_till(buff, '\n');
			q_buffer_move_gap_by(buff, to_newline + 1, Cursor_Dir_Right);
			u32 chars_in_next_line = runes_till(buff, '\n');
			u32 move_amount = Min(into_line, chars_in_next_line);
			q_buffer_move_gap_by(buff, move_amount, Cursor_Dir_Right);
		} else if (s_flags & Press_Up) {
			u32 into_line = runes_from(buff, '\n');
			q_buffer_move_gap_by(buff, into_line, Cursor_Dir_Left);
			if (buff->gap_index > 0) {
				q_buffer_move_gap_by(buff, 1, Cursor_Dir_Left);
				u32 prev_line_length = runes_from(buff, '\n');
				q_buffer_move_gap_by(buff, prev_line_length, Cursor_Dir_Left);
				u32 chars_in_prev_line = runes_till(buff, '\n');
				u32 move_amount = Min(into_line, chars_in_prev_line);
				q_buffer_move_gap_by(buff, move_amount, Cursor_Dir_Right);
			}
		}

		f32 x = 5, y = 5;
		f32 atlas_w = (f32)font_atlas.atlas_width;
		f32 atlas_h = (f32)font_atlas.atlas_height;
		f32 line_height = font_atlas_height(&font_atlas);

		Glyph_Info probe;
		font_atlas_get_glyph(&font_atlas, 'A', &probe);
		f32 mono_advance = probe.xadvance;

		String8 gap_buff_str = {
			.raw = buff->data,
			.len = buff->capacity,
		};

		bool in_line_comment = false;

		str8_foreach(gap_buff_str, itr, i) {
			bool on_cursor = false;
			if (i == buff->gap_index) {
				target_cursor_pos = (vec2_f32){x, y};
				cursor_pos.x = smooth_damp(cursor_pos.x, target_cursor_pos.x, 0.05f, (f32)delta_time);
				cursor_pos.y = smooth_damp(cursor_pos.y, target_cursor_pos.y, 0.05f, (f32)delta_time);

				usize idx = buff->gap_index + buff->gap_size;
				f32 x_scale = 1.0f;
				if (idx < buff->capacity && buff->data[idx] == '\t') {
					x_scale = 4.0f;
				}

				gfx_push_rect_rounded(&(Rect_Params){
					.position = {cursor_pos.x, cursor_pos.y},
						.color = 0x00ff00ff,
						.radii = rect_radius(mono_advance * 0.5),
						.size = {mono_advance * x_scale, line_height }
				});
				i = idx - itr.consumed;
				continue;
			}

			if (i == buff->gap_index + buff->gap_size) {
				on_cursor = true;
			}

			rune c = itr.codepoint;

			// Newline
			if (c == '\n') {
				in_line_comment = false;
				x = 5;
				y += line_height;
				if (y >= (f32)window_size.y) goto outside_string_itr;
				continue;
			}

			// Comment detection
			if (!in_line_comment && c == '/' && i + 1 < gap_buff_str.len) {
				usize peek = i + 1;
				if (peek >= buff->gap_index && peek < buff->gap_index + buff->gap_size) {
					peek += buff->gap_size;
				}
				if (peek < gap_buff_str.len && gap_buff_str.raw[peek] == '/') {
					in_line_comment = true;
				}
			}

			// Spaces / tabs -> monospaced advance
			if (c == ' ') {
				x += mono_advance;
				continue;
			}
			if (c == '\t') {
				x += mono_advance * 4.0f;
				continue;
			}

			// Glyph lookup
			Glyph_Info info;
			bool missing = false;
			if (!font_atlas_get_glyph(&font_atlas, c, &info)) {
				if (!font_atlas_get_glyph(&font_atlas, '?', &info)) {
					x += mono_advance;
					continue;
				}
				missing = true;
			}

			// Compute glyph position
			f32 glyph_x = x + info.xoff;
			f32 glyph_y = y + font_atlas.font.ascent + info.yoff;

			f32 gw = (f32)(info.x1 - info.x0);
			f32 gh = (f32)(info.y1 - info.y0);

			// Colors
			u32 col = 0x99856aff;
			if (on_cursor) col = 0x131313ff;
			else if (missing) col = 0xff0000ff;    
			else if (in_line_comment) col = 0x555555ff;
			

			// Draw glyph
			if (gw > 0 && gh > 0) {
				gfx_push_rect(&(Rect_Params){
					.position = {floorf(glyph_x), floorf(glyph_y)},
					.size     = {gw, gh},
					.color    = col,
					.uv       = {
						(f32)info.x0 / atlas_w,
						(f32)info.y0 / atlas_h,
						(f32)info.x1 / atlas_w,
						(f32)info.y1 / atlas_h
					},
					.tex_id   = font_tex_id,
				});
			}

			x += mono_advance;  // fixed monospaced advance
		}
	outside_string_itr:

		quark_frame_update(&quark_ctx, frame_input);
		gfx_end_frame();
		quark_window_swap_buff(quark_ctx.window);

	}

	font_atlas_delete(&quark_ctx.allocator, &font_atlas);

	quark_delete(&quark_ctx);

	return 0;
}
