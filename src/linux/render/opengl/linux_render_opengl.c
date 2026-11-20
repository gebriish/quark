
function void
r_ogl_os_init(void)
{
  global int fb_attributes[] =
  {
    GLX_X_RENDERABLE,   1,
    GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,    GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
    GLX_RED_SIZE,       8,
    GLX_GREEN_SIZE,     8,
    GLX_BLUE_SIZE,      8,
    GLX_ALPHA_SIZE,     8,
    GLX_DEPTH_SIZE,     24,
    GLX_STENCIL_SIZE,   8,
    GLX_DOUBLEBUFFER,   1,
    None
  };

  int fb_count = 0;
  GLXFBConfig *fb_configs = glXChooseFBConfig(lnx_gfx_state->display, DefaultScreen(lnx_gfx_state->display), fb_attributes, &fb_count);

  // @note: pick a config whose X visual is 32-bit (i.e. actually carries alpha)
  GLXFBConfig  fb_config    = fb_configs[0];
  XFree(fb_configs);
  {
    XVisualInfo *vi  =  glXGetVisualFromFBConfig(lnx_gfx_state->display, fb_config);
    lnx_gfx_state->visual = vi->visual;
    lnx_gfx_state->colormap = XCreateColormap(lnx_gfx_state->display,
                                              XRootWindow(lnx_gfx_state->display, vi->screen),
                                              vi->visual, AllocNone);
    XFree(vi);
  }

  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((u8 *)"glXCreateContextAttribsARB");
  int context_options[] =
  {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    None,
  };
  r_ogl_lnx_ctx = glXCreateContextAttribsARB(lnx_gfx_state->display, fb_config, 0, 1, context_options);
  glXMakeCurrent(lnx_gfx_state->display, 0, r_ogl_lnx_ctx);

  r_glXSwapIntervalEXT = (R_GLXSwapIntervalEXT)glXGetProcAddressARB((u8 *)"glXSwapIntervalEXT");
}

function void 
r_ogl_select_window(Win_Handle window)
{
  LNX_GFX_Window *w = (LNX_GFX_Window *) window.v[0];
  glXMakeCurrent(lnx_gfx_state->display, w->window, r_ogl_lnx_ctx);
  glClearColor(w->bg_color.r, w->bg_color.g, w->bg_color.b, w->bg_color.a);
  glDrawBuffer(GL_BACK);
}

function void 
r_window_set_vsync(Win_Handle win, b32 vsync)
{
  LNX_GFX_Window *w = (LNX_GFX_Window *) win.v[0];
  glXMakeCurrent(lnx_gfx_state->display, w->window, r_ogl_lnx_ctx);
  if (r_glXSwapIntervalEXT) r_glXSwapIntervalEXT(lnx_gfx_state->display, w->window, !!(vsync));
}

function void 
r_ogl_window_swap_buffers(Win_Handle window)
{
  LNX_GFX_Window *w = (LNX_GFX_Window *) window.v[0];
  glXSwapBuffers(lnx_gfx_state->display, w->window);
  window_sync_frame(window);
}
