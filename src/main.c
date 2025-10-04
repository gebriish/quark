////////////////////////////////
// ~geb: .h Includes
#include "base/base_inc.h"
#include "gfx/gfx_inc.h"

////////////////////////////////
// ~geb: thirdparty includes
#define RGFW_OPENGL
#define RGFW_IMPLEMENTATION
#include "thirdparty/rgfw.h"

////////////////////////////////
// ~geb: .c Includes
#include "base/base_inc.c"
#include "gfx/gfx_inc.c"

internal_lnk void
resize_callback(RGFW_window *window, i32 w, i32 h)
{
  GFX_State *gfx = (GFX_State *) RGFW_window_getUserPtr(window);
  gfx_resize_target(gfx, w, h);
}

int main() 
{
  Arena *arena = arena_alloc(MB(5));

  RGFW_glHints *hints = RGFW_getGlobalHints_OpenGL();
  hints->major = 3;
  hints->minor = 3;
  RGFW_setGlobalHints_OpenGL(hints);

  RGFW_window* window = RGFW_createWindow("LearnOpenGL", 0, 0, 1000, 625, RGFW_windowCenter | RGFW_windowOpenGL | RGFW_windowHide );
  if (window == NULL)
  {
    printf("Failed to create RGFW window\n");
    return -1;
  }
  RGFW_window_makeCurrentContext_OpenGL(window);
  RGFW_window_swapInterval_OpenGL(window, 0);

  if (!gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL)) {
    printf("Failed to initialize GLAD\n");
    return -1;
  }

  GFX_State gfx;
  gfx_init(arena, &gfx);
  gfx_resize_target(&gfx, 1000, 625);

  RGFW_window_setUserPtr(window, (void *)&gfx);
  RGFW_setWindowResizedCallback(resize_callback);

  RGFW_window_show(window);
  Time_Stamp curr_time = time_now(), last_time = 0;

  while (RGFW_window_shouldClose(window) == RGFW_FALSE) {
    last_time = curr_time;
    curr_time = time_now();
    f64 delta_time = time_diff(last_time, curr_time).seconds;

    RGFW_event event;
    while (RGFW_window_checkEvent(window, &event));

    i32 x, y;
    RGFW_window_getMouse(window, &x, &y);

    gfx_begin_frame(&gfx, 0x131313ff);
    gfx_push_rect(&gfx, (vec2_f32){x, y}, (vec2_f32){150, 150}, 0xff0000ff);
    gfx_end_frame(&gfx);

    RGFW_window_swapBuffers_OpenGL(window);
  }


  RGFW_window_close(window);
  arena_release(arena);
}
