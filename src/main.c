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

int main() 
{
  Quark_Context ctx;
  quark_new(&ctx);

  RGFW_window *window = ctx.window;

  Time_Stamp curr_time = time_now(), last_time = 0;

  while (quark_running(&ctx)) {
    last_time = curr_time;
    curr_time = time_now();
    f64 delta_time = time_diff(last_time, curr_time).seconds;
    LogInfo("%d", (i32)(1/delta_time));

    RGFW_event event;
    while (RGFW_window_checkEvent(window, &event));

    gfx_begin_frame(&ctx.gfx, 0x131313ff);
    gfx_push_rect(&ctx.gfx, (vec2_f32){20,20}, (vec2_f32){100,100}, 0xebdbc7ff);
    gfx_end_frame(&ctx.gfx);

    RGFW_window_swapBuffers_OpenGL(window);
  }

  quark_delete(&ctx);
}
