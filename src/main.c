////////////////////////////////main
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
			18,
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

	while (quark_window_is_open(quark_window)) {
		arena_clear(quark_ctx.transient_arena);
		Time_Stamp frame_start = time_now();
		f64 delta_time = time_diff(last_time, frame_start).seconds;
		last_time = frame_start;

		LogInfo("%d", (int)(1.0/delta_time));

		Input_Data frame_input = quark_gather_input(quark_window);

		DeferScope(gfx_begin_frame(0x131313ff), gfx_end_frame()) {
			push_rect(
				.size={font_atlas.dim.x, font_atlas.dim.y}, 
				.tex_id = font_tex_id, 
				.color = 0x99856aff
			);
		}

		quark_window_swap_buff(quark_window);
	}

	return 0;
}
