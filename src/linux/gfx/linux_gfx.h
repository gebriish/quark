#ifndef LINUX_GFX_H
#define LINUX_GFX_H

#undef function // `function` is used in Xorg
# include <X11/Xlib.h>
# include <X11/cursorfont.h>
# include <X11/Xatom.h>
# include <X11/extensions/sync.h>
# include <X11/keysym.h>
#define function static

typedef struct LNX_GFX_Window LNX_GFX_Window;
struct LNX_GFX_Window
{
  LNX_GFX_Window *prev;
  LNX_GFX_Window *next;

  S32     width;
  S32     height;
  Vec4F32 bg_color;

  Window window;
  XIC xic;
  XID counter_xid;
  U64 counter_value;
  B32 sync_pending;
  B32 custom_titlebar;
  F32 move_hit_region[Edge_COUNT];
};

typedef struct LNX_GFX_WindowList LNX_GFX_WindowList;
struct LNX_GFX_WindowList
{
  LNX_GFX_Window *first;
  LNX_GFX_Window *last;
  S64 len;
};

typedef struct LNX_GFX_State LNX_GFX_State;
struct LNX_GFX_State
{
  Arena *arena;
  Display *display;
  Visual  *visual;
  Colormap colormap;
  int depth;
  float x;
  XIM xim;
  Atom wm_clipboard;
  Atom wm_protocols;
  Atom wm_delete_window;
  Atom wm_sync_request;
  Atom wm_sync_req_counter;
  LNX_GFX_WindowList windows;
  LNX_GFX_Window *free_windows;
  GFX_Modifiers modifiers;
  U64 key_down_state[(GFX_Key_COUNT + 63) / 64];
  Cursor xcursors[GFX_Cursor_COUNT];
};

global LNX_GFX_State *lnx_gfx_state = 0;

////////////////////////////////////////////////
// @section: local linux gfx helpers

function LNX_GFX_Window *lnx_window_from_xwindow(Window window);
function GFX_Key lnx_keycode_from_keysym(KeySym ks);

#endif // LINUX_GFX_H
