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
			16,
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

	arena_print_usage(quark_ctx.persist_arena, "persist_arena");

	Time_Stamp last_time = time_now();

	Buffer_Manager buf_man;
	buffer_manager_init(quark_ctx.persist_arena, &buf_man);

	Gap_Buffer *buff = gap_buffer_new(&buf_man, KB(16));

	while (quark_window_is_open(quark_window)) {
		arena_clear(quark_ctx.transient_arena);
		Time_Stamp frame_start = time_now();
		f64 delta_time = time_diff(last_time, frame_start).seconds;
		last_time = frame_start;

		Input_Data frame_input = quark_gather_input(quark_window);

		Special_Press_Flags s_flags = frame_input.special_press;
		if (s_flags & Special_Press_Return) {
			gap_buffer_insert(buff, str8_lit("\n"));
		}else if(s_flags & Special_Press_Backspace) {
			gap_buffer_delete_rune(buff, 1, Cursor_Dir_Left);
		}else if(s_flags & Special_Press_Delete) {
			gap_buffer_delete_rune(buff, 1, Cursor_Dir_Right);
		}else if(s_flags & Special_Press_Tab) {
			gap_buffer_insert(buff, str8_lit("\t"));
		}else if(s_flags & Special_Press_L) {
			gap_buffer_move_gap_by(buff, 1, Cursor_Dir_Left);
		}else if(s_flags & Special_Press_R) {
			gap_buffer_move_gap_by(buff, 1, Cursor_Dir_Right);
		}else if(s_flags & Special_Copy) {
			const char *cstr = glfwGetClipboardString((GLFWwindow *)quark_window);
			String8 clipboard_string = str8_lit("");
			if (cstr) {
				clipboard_string = str8_cstr_slice(cstr, 0, -1);
			}
			gap_buffer_insert(buff, clipboard_string);
		}else if (frame_input.codepoint) {
			u8 mem[4] = {0};
			String8 encoded_rune = str8_encode_rune(frame_input.codepoint, mem);
			gap_buffer_insert(buff, encoded_rune);
		}

		String8 left_string = {
			.raw = buff->data,
			.len = buff->gap_index
		};
		String8 right_string = {
			.raw = buff->data + buff->gap_index + buff->gap_size,
			.len = buff->capacity - (buff->gap_index + buff->gap_size)
		};

		gfx_begin_frame(0x131313ff); 

		f32 x = 0, y = 0;
		f32 atlas_w = (f32)font_atlas.dim.x;
		f32 atlas_h = (f32)font_atlas.dim.y;
		f32 scale = font_metrics_scale(&font_atlas.metrics);
		f32 baseline_offset = font_atlas.metrics.ascent * scale;
		f32 mono_advance = (f32)font_atlas.metrics.font_size * 0.7f;
		f32 height = font_metrics_line_height(&font_atlas.metrics) * font_metrics_scale(&font_atlas.metrics);

		String8 gap_buff_str = {
			.raw = buff->data,
			.len = buff->capacity,
		};

		local_persist vec2_f32 cursor_pos = {0};
		local_persist vec2_f32 target_cursor_pos;

		str8_foreach(gap_buff_str, itr, i) {
			color8_t col = 0x928374ff;
			if (i >= buff->gap_index && i < buff->gap_index + buff->gap_size) {
				if (i == buff->gap_index) {
					target_cursor_pos.x = x;
					target_cursor_pos.y = y;
				}
				i = buff->gap_index + buff->gap_size - itr.consumed;

				cursor_pos.x = smooth_damp(cursor_pos.x, target_cursor_pos.x, 0.05f, (f32)delta_time);
				cursor_pos.y = smooth_damp(cursor_pos.y, target_cursor_pos.y, 0.05f, (f32)delta_time);

				push_rect_rounded(
					.position = cursor_pos,
					.color = 0xb8bb26ff,
					.radii = rect_radius(99),
					.size  = {mono_advance, height}
				);
				continue;
			}
			if (i == buff->gap_index + buff->gap_size) {
				col = 0x131313ff;
			}

			rune codepoint = itr.codepoint;
			if (codepoint == ' ') {
				x += mono_advance;
				continue;
			}

			if (codepoint == '\n') {
				x = 0;
				y += height;
				continue;
			}

			if (codepoint == '\t') {
				x += mono_advance * 4.0f;
				continue;
			}

			Glyph_Info info;
			bool ok = glyph_map_get(font_atlas.code_to_glyph, codepoint, &info);
			if (!ok) {
				col = 0xff0000ff;
				ok = glyph_map_get(font_atlas.code_to_glyph, '?', &info);
				if (!ok) {
					x += mono_advance;
					continue;
				}
			}

			f32 glyph_x = x + (mono_advance - (f32)info.size.x) * 0.5f + (f32)info.bearing.x * 0.0f;
			f32 glyph_y = y + baseline_offset - (f32)info.bearing.y;

			f32 u0 = (f32)info.position.x / atlas_w;
			f32 v0 = (f32)info.position.y / atlas_h;
			f32 u1 = ((f32)info.position.x + (f32)info.size.x) / atlas_w;
			f32 v1 = ((f32)info.position.y + (f32)info.size.y) / atlas_h;

			if (info.size.x > 0 && info.size.y > 0) {
				Rect_Params rect = {
					.position = {floorf(glyph_x), floorf(glyph_y)},
					.size     = {(f32)info.size.x, (f32)info.size.y},
					.color    = col,
					.uv       = {u0, v0, u1, v1},
					.tex_id   = font_tex_id,
				};
				gfx_push_rect(&rect);
			}

			x += mono_advance;
		}

		gfx_end_frame();

		quark_window_swap_buff(quark_window);
	}

	return 0;
}
