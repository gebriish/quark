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

#define TARGET_FRAME_RATE 120
#define TARGET_FRAME_TIME (1000.0/TARGET_FRAME_RATE)

int main() 
{
  Quark_Context ctx;
  quark_new(&ctx);
  RGFW_window *window = ctx.window;

  font_init(ctx.persist_arena, str8_lit("./jetbrains_mono.ttf"));

  Temp tmp = temp_begin(ctx.persist_arena);
  vec2_u16 atlas_size; 
  { // no need to store atlas image in CPU after GPU upload
    Font_Atlas atlas = font_generate_atlas(
      ctx.persist_arena,
      20, 
      str8_lit(
        FONT_CHARSET
      )
    );
    atlas_size = atlas.dim;
    Texture_Data data = {
      .data = atlas.image,
      .width = atlas.dim.x,
      .height = atlas.dim.y,
      .channels = 1,
    };
    gfx_texture_upload(&ctx.gfx, data, TextureKind_GreyScale);
  }
  temp_end(tmp);


  arena_print_usage(ctx.persist_arena, "persist_arena");
  arena_print_usage(ctx.frame_arena, "frame_arena");

  Time_Stamp last_time = time_now();
  while (quark_running(&ctx)) {
    Time_Stamp frame_start = time_now();
    Time_Duration diff = time_diff(last_time, frame_start);
    f64 delta_time = diff.seconds;

    RGFW_pollEvents();
    arena_clear(ctx.frame_arena);

    local_persist i32 tx, ty;
    if (RGFW_isMouseDown(RGFW_mouseLeft)) {
      RGFW_window_getMouse(window, &tx, &ty);
    }

    local_persist f32 x, y;
    x = smooth_damp(x, tx, 0.07, delta_time);
    y = smooth_damp(y, ty, 0.07, delta_time);
   
    gfx_begin_frame(&ctx.gfx, 0x131313ff);
    push_rect_rounded(&ctx.gfx, .position = {x,y-10}, .radii = {5,5,5,5}, .size = {100,100}, .color=0x40ff40ff);
    push_rect(&ctx.gfx, .position = {20,20}, .size = {atlas_size.x, atlas_size.y}, .color=0x99856aff, .tex_id = 1);
    gfx_end_frame(&ctx.gfx);
    
    RGFW_window_swapBuffers_OpenGL(window);

    Time_Stamp frame_end = time_now();
    Time_Duration frame_time = time_diff(frame_start, frame_end);

    if (frame_time.milliseconds < TARGET_FRAME_TIME) {
      time_sleep_ms((u32)(TARGET_FRAME_TIME - frame_time.milliseconds));
    }

    last_time = frame_start;
  }

  quark_delete(&ctx);
}
