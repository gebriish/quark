#include "quark_core.h"

internal_lnk void
resize_callback(RGFW_window *window, i32 w, i32 h)
{
  Quark_Context *ctx = (Quark_Context *) RGFW_window_getUserPtr(window);
  gfx_resize_target(&ctx->gfx, w, h);
}

internal_lnk void
quark_new(Quark_Context *context)
{
  Assert(context && "quark context ptr is null");

  context->persist_arena = arena_alloc(MB(5));
  context->frame_arena   = arena_alloc(MB(5));

  RGFW_glHints *hints = RGFW_getGlobalHints_OpenGL();
  hints->major = 3;
  hints->minor = 3;
  RGFW_setGlobalHints_OpenGL(hints);

  RGFW_window *window = RGFW_createWindow("quark", 0, 0, 1000, 625, RGFW_windowCenter | RGFW_windowOpenGL | RGFW_windowHide );
  if (window == NULL)
  {
    LogError("Failed to create RGFW window\n");
    return;
  }
  context->window = window;
  RGFW_window_makeCurrentContext_OpenGL(window);
  RGFW_window_swapInterval_OpenGL(window, 0);

  if (!gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL)) {
    LogError("Failed to initialize GLAD\n");
    return;
  }

  if (!gfx_init(context->persist_arena, &context->gfx)) {
    LogError("Failed to Initialize GFX");
    return;
  }
  gfx_resize_target(&context->gfx, 1000, 625);

  RGFW_window_setUserPtr(window, (void *)(context));
  RGFW_setWindowResizedCallback(resize_callback);

  RGFW_window_show(window);
}

internal_lnk void
quark_delete(Quark_Context *context)
{
  Assert(context && "quark context ptr is null");
  
  RGFW_window_close(context->window);

  arena_release(context->persist_arena);
  arena_release(context->frame_arena);
}


internal_lnk bool
quark_running(Quark_Context *context)
{
  return !RGFW_window_shouldClose(context->window);
}
