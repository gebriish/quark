////////////////////////////////
// ~geb: .h Includes
#include "base/base_inc.h"
#include "gfx/gfx_inc.h"
#include "quark/quark_inc.h"

////////////////////////////////
// ~geb: .c Includes
#include "base/base_inc.c"
#include "gfx/gfx_inc.c"
#include "quark/quark_inc.c"

global Quark_Context quark_ctx;
global Quark_Window  quark_window;
global GFX_State     gfx_state;
global Font_Atlas    font_atlas;
global u8            font_tex_id;

int main(void)
{
	// ~geb: Layer initializations
	quark_new(&quark_ctx);
	quark_window = quark_window_open();
	gfx_init(quark_ctx.persist_arena, quark_ctx.transient_arena, &gfx_state, quark_get_ogl_proc_addr);
	font_init(quark_ctx.persist_arena, str8_lit("./res/fonts/JetBrainsMono-Regular.ttf"));

	{
		// ~geb: Font texture atlas generation
		font_atlas = font_generate_atlas(
			quark_ctx.persist_arena,
			20,
			str8_lit(FONT_CHARSET)
		);
		u8 *image = font_rasterize_atlas(quark_ctx.transient_arena, &font_atlas);
		Texture_Data data = {
			.data = image,
			.width = font_atlas.dim.x,
			.height = font_atlas.dim.y,
			.channels = 1,
		};
		font_tex_id = (u8) gfx_texture_upload(data, TextureKind_GreyScale);
	}

	Time_Stamp last_time = time_now();

	Buffer_Manager bm;
	buffer_manager_init(quark_ctx.persist_arena, &bm);

	Gap_Buffer *buff = gap_buffer_new(&bm, KB(10));

	while (quark_window_is_open(quark_window)) {
		arena_clear(quark_ctx.transient_arena);
		Time_Stamp frame_start = time_now();
		f64 delta_time = time_diff(last_time, frame_start).seconds;
		last_time = frame_start;

		Input_Data frame_input = quark_gather_input(quark_window);
		quark_frame_update(&quark_ctx, frame_input);

		Special_Press_Flags s_flags = frame_input.special_press;

		gfx_begin_frame(0x131313ff);

		if (quark_ctx.state == Quark_State_Insert) {
			if (s_flags & Special_Press_Enter) {
				gap_buffer_insert(buff, str8_lit("\n"));
			}else if(s_flags & Special_Press_Backspace) {
				gap_buffer_delete_rune(buff, 1, Cursor_Dir_Left);
			}else if(s_flags & Special_Press_Delete) {
				gap_buffer_delete_rune(buff, 1, Cursor_Dir_Right);
			}else if(s_flags & Special_Press_Tab) {
				gap_buffer_insert(buff, str8_lit("\t"));
			}else if(s_flags & Special_Press_Left) {
				gap_buffer_move_gap_by(buff, 1, Cursor_Dir_Left);
			}else if(s_flags & Special_Press_Right) {
				gap_buffer_move_gap_by(buff, 1, Cursor_Dir_Right);
			}else if(s_flags & Special_Press_Down) {
				u32 into_line = runes_from(buff, '\n');
				u32 to_newline = runes_till(buff, '\n');
				gap_buffer_move_gap_by(buff, to_newline + 1, Cursor_Dir_Right);
				u32 chars_in_next_line = runes_till(buff, '\n');
				u32 move_amount = Min(into_line, chars_in_next_line);
				gap_buffer_move_gap_by(buff, move_amount, Cursor_Dir_Right);
			}else if(s_flags & Special_Press_Up) {
				u32 into_line = runes_from(buff, '\n');
				gap_buffer_move_gap_by(buff, into_line, Cursor_Dir_Left);
				if (buff->gap_index > 0) {
					gap_buffer_move_gap_by(buff, 1, Cursor_Dir_Left);
					u32 prev_line_length = runes_from(buff, '\n');
					gap_buffer_move_gap_by(buff, prev_line_length, Cursor_Dir_Left);
					u32 chars_in_prev_line = runes_till(buff, '\n');
					u32 move_amount = Min(into_line, chars_in_prev_line);
					gap_buffer_move_gap_by(buff, move_amount, Cursor_Dir_Right);
				}
			}
			else if (frame_input.codepoint) {
				u8 mem[4] = {0};
				String8 encoded_rune = str8_encode_rune(frame_input.codepoint, mem);
				gap_buffer_insert(buff, encoded_rune);
			}
		}

		f32 x = 0, y = 0;
		f32 atlas_w = (f32)font_atlas.dim.x;
		f32 atlas_h = (f32)font_atlas.dim.y;
		f32 scale = font_metrics_scale(&font_atlas.metrics);
		f32 baseline_offset = font_atlas.metrics.ascent * scale;
		f32 height = font_metrics_line_height(&font_atlas.metrics) * scale;

		Glyph_Info space_info;
		f32 space_advance = (f32) font_atlas.metrics.font_size * 0.7f;

		String8 gap_buff_str = {
			.raw = buff->data,
			.len = buff->capacity,
		};

		local_persist vec2_f32 cursor_pos = {0};
		local_persist vec2_f32 target_cursor_pos = {0};

		bool in_line_comment = false;

		str8_foreach(gap_buff_str, itr, i) {
			bool on_cursor = false;

			if (i >= buff->gap_index && i < buff->gap_index + buff->gap_size) {
				if (i == buff->gap_index) {
					target_cursor_pos = (vec2_f32){x, y};
				}
				i = buff->gap_index + buff->gap_size - itr.consumed;
				cursor_pos.x = smooth_damp(cursor_pos.x, target_cursor_pos.x, 0.06f, (f32)delta_time);
				cursor_pos.y = smooth_damp(cursor_pos.y, target_cursor_pos.y, 0.06f, (f32)delta_time);
				push_rect_rounded(
					.position = {floorf(cursor_pos.x), floorf(cursor_pos.y)},
					.color = 0x00ff00ff,
					.radii = rect_radius(space_advance * 0.5f),
					.size = {space_advance, height}
				);
				continue;
			}
			if (i == buff->gap_index + buff->gap_size) {
				on_cursor = true;
			}

			rune c = itr.codepoint;

			if (c == '\n') { 
				in_line_comment = false;
				x = 0; 
				y += height; 
				continue; 
			}

			if (!in_line_comment && c == '/' && i + 1 < gap_buff_str.len) {
				size_t peek_idx = i + 1;
				if (peek_idx >= buff->gap_index && peek_idx < buff->gap_index + buff->gap_size) {
					peek_idx += buff->gap_size;
				}
				if (peek_idx < gap_buff_str.len && gap_buff_str.raw[peek_idx] == '/') {
					in_line_comment = true;
				}
			}

			if (c == ' ')  { x += space_advance; continue; }
			if (c == '\t') { x += space_advance * 4; continue; }

			Glyph_Info info;
			bool char_missing = false;
			if (!glyph_map_get(font_atlas.code_to_glyph, c, &info)) {
				if (!glyph_map_get(font_atlas.code_to_glyph, '?', &info)) {
					x += space_advance;
					continue;
				} 
				char_missing = true;
			}
			if (info.size.x > 0 && info.size.y > 0) {
				f32 glyph_x = x + (f32)info.bearing.x;
				f32 glyph_y = y + baseline_offset - (f32)info.bearing.y;

				u32 text_color = char_missing ? 0xff0000ff : (on_cursor ? 0x131313ff : (in_line_comment ? 0x676767ff : 0x99856aff));

				gfx_push_rect(&(Rect_Params){
					.position = {floorf(glyph_x), floorf(glyph_y)},
					.size = {(f32)info.size.x, (f32)info.size.y},
					.color = text_color,
					.uv = {
						(f32)info.position.x / atlas_w,
						(f32)info.position.y / atlas_h,
						((f32)info.position.x + (f32)info.size.x) / atlas_w,
						((f32)info.position.y + (f32)info.size.y) / atlas_h
					},
					.tex_id = font_tex_id,
				});
			}
			x += (f32)space_advance;
		}
		gfx_end_frame();

		quark_window_swap_buff(quark_window);
	}

	quark_delete(&quark_ctx);

	return 0;
}
