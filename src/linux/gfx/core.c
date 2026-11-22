
function void
gfx_init(void)
{
  Arena *arena = arena_reserve(Megabytes(4));
  lnx_gfx_state = arena_push(arena, LNX_GFX_State, 1);

  Display *display = XOpenDisplay(0);

  lnx_gfx_state->arena = arena;
  lnx_gfx_state->display = display;
  lnx_gfx_state->xim = XOpenIM(display, 0, 0, 0);

  lnx_gfx_state->wm_clipboard        = XInternAtom(display, "CLIPBOARD",                    0);
  lnx_gfx_state->wm_protocols        = XInternAtom(display, "WM_PROTOCOLS",                 0);
  lnx_gfx_state->wm_delete_window    = XInternAtom(display, "WM_DELETE_WINDOW",             0);
  lnx_gfx_state->wm_sync_request     = XInternAtom(display, "_NET_WM_SYNC_REQUEST",         0);
  lnx_gfx_state->wm_sync_req_counter = XInternAtom(display, "_NET_WM_SYNC_REQUEST_COUNTER", 0);
}

function Win_Handle 
window_open(String8 title, Vec2F32 resolution, Win_Flags flags)
{
  Temp scratch = scratch_begin(0, 0);

  // allocate new window

  LNX_GFX_Window *lnx_win = lnx_gfx_state->free_windows;
  if (lnx_gfx_state->free_windows)
  {
    SLLStackPop(lnx_gfx_state->free_windows);
  }
  else 
  {
    lnx_win = arena_push_non_zero(lnx_gfx_state->arena, LNX_GFX_Window, 1);
  }

  MemZeroStruct(lnx_win);
  DLLPushBack(lnx_gfx_state->windows.first, lnx_gfx_state->windows.last, lnx_win);
  lnx_gfx_state->windows.len += 1;

  // setup Xlib stuff for window

  Display *display  = lnx_gfx_state->display;
  Visual *visual    = lnx_gfx_state->visual;
  Colormap colormap = lnx_gfx_state->colormap;
  int depth         = lnx_gfx_state->depth;
  unsigned long attr_mask = 0;

  XSetWindowAttributes window_attribs = {0};
  if (visual == 0)
  {
    visual = (Visual *)CopyFromParent;
    depth  = CopyFromParent;
  }
  else
  {
    window_attribs.background_pixmap = None;
    window_attribs.border_pixel      = 0;
    window_attribs.colormap          = colormap;
    attr_mask = CWBackPixmap | CWBorderPixel | CWColormap;
  }

  // @todo(gaureesh): take flags into consideration while making window
  lnx_win->window = XCreateWindow(display,
                                  XDefaultRootWindow(display),
                                  0, 0, (u32) resolution.x, (u32) resolution.y,
                                  0,
                                  depth,
                                  InputOutput,
                                  visual,
                                  attr_mask,
                                  &window_attribs);

  long event_mask = ExposureMask        | PointerMotionMask | ButtonPressMask | 
                    ButtonReleaseMask   | KeyPressMask      | KeyReleaseMask  |
                    StructureNotifyMask | FocusChangeMask   | LeaveWindowMask;


  XSelectInput(display, lnx_win->window, event_mask);

  XSetWindowBackgroundPixmap(display, lnx_win->window, None);
  Atom protocols[] =
  {
    lnx_gfx_state->wm_delete_window,
    lnx_gfx_state->wm_sync_request,
  };
  XSetWMProtocols(display, lnx_win->window, protocols, CArrayLength(protocols));
  {
    XSyncValue initial_value;
    XSyncIntToValue(&initial_value, 0);
    lnx_win->counter_xid = XSyncCreateCounter(display, initial_value);
  }
  XChangeProperty(display, lnx_win->window, lnx_gfx_state->wm_sync_req_counter, XA_CARDINAL, 32, PropModeReplace, (u8 *)&lnx_win->counter_xid, 1);

  lnx_win->xic = XCreateIC(lnx_gfx_state->xim,
                           XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
                           XNClientWindow, lnx_win->window,
                           XNFocusWindow, lnx_win->window,
                           NULL);

  const char *title_cstr = (char *) str8_push_cstring(scratch.arena, title);
  XStoreName(display, lnx_win->window, title_cstr);

  if (flags & Win_Flag_CustomBorder)
  {
    struct
    {
      u32 flags;
      u32 functions;
      u32 decorations;
      s32 inputMode;
      u32 status;
    } hints = {0};

    hints.flags = 2;
    hints.decorations = 0;
    Atom property = XInternAtom(display, "_MOTIF_WM_HINTS", 1);
    if(property != None)
    {
      XChangeProperty(display, lnx_win->window, property, property, 32, PropModeReplace, (unsigned char *) &hints, 5);
    }
    lnx_win->custom_titlebar = true;
  }


  XMapWindow(display, lnx_win->window);
  XFlush(display);

  struct { Win_Cursor cursor; unsigned int id; }
  map[] =
  {
    {Win_Cursor_Pointer,             XC_left_ptr},
    {Win_Cursor_IBar,                XC_xterm},
    {Win_Cursor_LeftRight,           XC_sb_h_double_arrow},
    {Win_Cursor_UpDown,              XC_sb_v_double_arrow},
    {Win_Cursor_UpLeft,              XC_top_left_corner},
    {Win_Cursor_UpRight,             XC_top_right_corner},
    {Win_Cursor_DownLeft,            XC_bottom_left_corner},
    {Win_Cursor_DownRight,           XC_bottom_right_corner},
    {Win_Cursor_UpDownLeftRight,     XC_fleur},
    {Win_Cursor_HandPoint,           XC_hand2},
    {Win_Cursor_HandGrab,            XC_hand1},
    {Win_Cursor_Disabled,            XC_X_cursor},
  };
  for each_index(idx, Win_Cursor_COUNT)
  {
    lnx_gfx_state->xcursors[map[idx].cursor] = XCreateFontCursor(display, map[idx].id);
  }

  scratch_end(scratch);
  Win_Handle result = {(u64) lnx_win};
  return result;
}

function void
window_close(Win_Handle window)
{
  if (window_zero(window)) 
  {
    return;
  }
  // @todo(gaureesh): confirm window is in list ? 

  LNX_GFX_Window *win = (LNX_GFX_Window *) window.v[0];
  XDestroyWindow(lnx_gfx_state->display, win->window);
  DLLRemove(lnx_gfx_state->windows.first, lnx_gfx_state->windows.last, win);
  SLLStackPush(lnx_gfx_state->free_windows, win);
}

function Win_Event_List 
get_window_events(Arena *arena)
{
  Win_Event_List event_list = {0};
  while (XPending(lnx_gfx_state->display))
  {
    XEvent event = {0};
    XNextEvent(lnx_gfx_state->display, &event);

    b32 set_mouse_cursor = false;
    switch (event.type)
    {
      default: {} break;

      case LeaveNotify:
      {
        LNX_GFX_Window *window = lnx_window_from_xwindow(event.xcrossing.window);
        if (window != 0 && window->custom_titlebar)
        {
          set_mouse_cursor = true;
          set_cursor_icon(Win_Cursor_Pointer);
        }
      } break;

      case ConfigureNotify:
      {
        LNX_GFX_Window *w = lnx_window_from_xwindow(event.xconfigure.window);

        if (w->width != event.xconfigure.width || w->height != event.xconfigure.height)
        {
          w->width = event.xconfigure.width;
          w->height = event.xconfigure.height;

          Win_Event *e = window_event_push(arena, &event_list, Win_Event_Kind_Resize);
          e->window.v[0] = (u64) w;
          e->size = v2f32((f32) w->width, (f32) w->height);

          Atom net_wm_opaque_region = XInternAtom(lnx_gfx_state->display, "_NET_WM_OPAQUE_REGION", False);
          long data[4] = {
            0, 0,
            (long) w->width,
            (long) w->height,
          };
          XChangeProperty( lnx_gfx_state->display, 
                           w->window, 
                           net_wm_opaque_region, 
                           XA_CARDINAL,
                           32,
                           PropModeReplace,
                           (unsigned char *)data, 
                           CArrayLength(data) );
        }

      } break;

      case KeyPress:
      case KeyRelease:
      {
        Key_Modifiers mods = 0;
        if(event.xkey.state & ShiftMask)   { mods |= Key_Modifier_Shift; }
        if(event.xkey.state & ControlMask) { mods |= Key_Modifier_Ctrl; }
        if(event.xkey.state & Mod1Mask)    { mods |= Key_Modifier_Alt; }

        LNX_GFX_Window *window = lnx_window_from_xwindow(event.xkey.window);
        KeySym keysym   = XLookupKeysym(&event.xkey, 0);
        Key keycode = lnx_keycode_from_keysym(keysym);

        s32 text_size = 0;
        static u8 text[256] = {0};

        if (event.type == KeyPress && window != 0)
        {
          KeySym ignored = 0;
          text_size = Xutf8LookupString(window->xic, &event.xkey, (char *)text, sizeof(text), &ignored, 0);
        }

        if (event.type == KeyPress)
        {
          lnx_gfx_state->modifiers |= mods;
          lnx_gfx_state->key_down_state[keycode >> 6] |= (1uLL << (keycode & 0x3F));
        }
        else
        {
          lnx_gfx_state->modifiers &= ~mods;
          lnx_gfx_state->key_down_state[keycode >> 6] &= ~(1uLL << (keycode & 0x3F));
        }

        if (event.type == KeyPress && text_size != 0)
        {
          String8 string = { text, text_size };
          for (s64 i=0; i<text_size;)
          {
            s32 increment = 0;
            u32 codepoint = utf8_decode(string, i, &increment);
            if (codepoint != 0 && codepoint != 127 && codepoint >= 32)
            {
              Win_Event *e = window_event_push(arena, &event_list, Win_Event_Kind_Text);
              e->window.v[0] = (u64) window;
              e->character = codepoint;
            }
            if (increment == 0) 
            {
              break;
            }
            i += increment;
          }
        }

        Win_Event *e = window_event_push(arena, &event_list, event.type == KeyPress ? Win_Event_Kind_Press : Win_Event_Kind_Release);
        e->window.v[0] = (u64) window;
        e->modifiers = mods;
        e->keycode = keycode;
      } break;

      case ButtonPress:
      case ButtonRelease:
      {
        LNX_GFX_Window *window = lnx_window_from_xwindow(event.xbutton.window);

        Win_Handle win = {(u64)window};
        Vec2F32 button_pos = v2f32((f32)event.xbutton.x, (f32)event.xbutton.y);

        b32 in_client = false;
        Win_Region moveresize_code = window_region_from_pixel(win, button_pos, &in_client);
        if (!in_client && window->custom_titlebar)
        {
          XUngrabPointer(lnx_gfx_state->display, event.xbutton.time);
          Atom net_wm_moveresize = XInternAtom(lnx_gfx_state->display, "_NET_WM_MOVERESIZE", 0);

          XEvent xev;
          MemZeroStruct(&xev);
          {
            xev.xclient.type = ClientMessage;
            xev.xclient.display = lnx_gfx_state->display;
            xev.xclient.window = window->window;
            xev.xclient.message_type = net_wm_moveresize;
            xev.xclient.format = 32;
            xev.xclient.data.l[0] = event.xbutton.x_root;
            xev.xclient.data.l[1] = event.xbutton.y_root;
            xev.xclient.data.l[2] = moveresize_code;
          }

          XSendEvent(lnx_gfx_state->display, DefaultRootWindow(lnx_gfx_state->display), 0, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
          XFlush(lnx_gfx_state->display);
        }
        else
        {
          Key_Modifiers mods = 0;
          if(event.xbutton.state & ShiftMask)   { mods |= Key_Modifier_Shift; }
          if(event.xbutton.state & ControlMask) { mods |= Key_Modifier_Ctrl; }
          if(event.xbutton.state & Mod1Mask)    { mods |= Key_Modifier_Alt; }

          Key key = Key_Null;
          switch (event.xbutton.button)
          {
            default: break;
            case Button1: key = Key_LeftMouseButton; break;
            case Button2: key = Key_MiddleMouseButton; break;
            case Button3: key = Key_RightMouseButton; break;
          }

          if (key != Key_Null)
          {
            Win_Event *e = window_event_push(arena, &event_list, event.type == ButtonPress ? Win_Event_Kind_Press : Win_Event_Kind_Release);
            e->window.v[0] = (u64) window;
            e->modifiers = mods;
            e->keycode = key;
            e->pos = button_pos;

            if (event.type == ButtonPress)
            {
              lnx_gfx_state->modifiers |= mods;
              lnx_gfx_state->key_down_state[key >> 6] |= (1uLL << (key & 0x3F));
            }
            else
            {
              lnx_gfx_state->modifiers &= ~mods;
              lnx_gfx_state->key_down_state[key >> 6] &= ~(1uLL << (key & 0x3F));
            }

          }
          else if (event.xbutton.button == Button4 ||
              event.xbutton.button == Button5 ||
              event.xbutton.button == 6 ||
              event.xbutton.button == 7)
          {
            Win_Event *e = window_event_push(arena, &event_list, Win_Event_Kind_Scroll);
            e->window.v[0] = (u64) window;
            e->modifiers = mods;
            e->delta = v2f32(.x = event.xbutton.button == 6 ? -120.f : event.xbutton.button == 7 ? +120.f : 0.f,
                             .y = event.xbutton.button == Button4 ? -120.f : event.xbutton.button == Button5 ? +120.f : 0.f);
            e->pos = button_pos;
          }
        }

      } break;

      case MotionNotify:
      {
        LNX_GFX_Window *window = lnx_window_from_xwindow(event.xmotion.window);

        Win_Handle win_h  = (Win_Handle) {(u64) window};
        Vec2F32 mouse_pos = v2f32((f32) event.xmotion.x, (f32) event.xmotion.y);;

        Win_Event *e   = window_event_push(arena, &event_list, Win_Event_Kind_MouseMove);
        e->window.v[0] = (u64) window;
        e->pos         = mouse_pos;

        set_mouse_cursor = true;

        b32 in_client = false;
        Win_Region moveresize_code = window_region_from_pixel(win_h, mouse_pos, &in_client);
        if (window->custom_titlebar)
        {
          if (!in_client) 
          {
            Win_Cursor cursor = window_region_to_cursor(moveresize_code);
            set_cursor_icon(cursor);
          }
          else
          {
            set_cursor_icon(Win_Cursor_Pointer);
          }
        }

      } break;

      case ClientMessage:
      {
        if (event.xclient.message_type != lnx_gfx_state->wm_protocols)
        {
          break;
        }

        Atom protocol = (Atom) event.xclient.data.l[0];
        if(protocol == lnx_gfx_state->wm_delete_window)
        {
          LNX_GFX_Window *window = lnx_window_from_xwindow(event.xclient.window);
          Win_Event *e = window_event_push(arena, &event_list, Win_Event_Kind_Close);
          e->window.v[0] = (u64) window;
        }
        else if(protocol == lnx_gfx_state->wm_sync_request)
        {
          LNX_GFX_Window *window = lnx_window_from_xwindow(event.xclient.window);
          if(window != 0)
          {
            u32 counter_low = (u32)event.xclient.data.l[2];
            u32 counter_hi  = (u32)event.xclient.data.l[3];
            window->counter_value = (counter_low | ((u64)counter_hi << 32));
            window->sync_pending = 1;
          }
        }
      } break;
    }

    if (set_mouse_cursor)
    {
      Window root_window = 0;
      Window child_window = 0;
      int root_rel_x = 0;
      int root_rel_y = 0;
      int child_rel_x = 0;
      int child_rel_y = 0;
      unsigned int mask = 0;
      if(XQueryPointer(lnx_gfx_state->display, XDefaultRootWindow(lnx_gfx_state->display), &root_window, &child_window, &root_rel_x, &root_rel_y, &child_rel_x, &child_rel_y, &mask))
      {
        XDefineCursor(lnx_gfx_state->display, root_window, lnx_gfx_state->xcursors[lnx_gfx_state->last_set_cursor]);
        XFlush(lnx_gfx_state->display);
      }
    }
  }
  return event_list;
}

function LNX_GFX_Window *
lnx_window_from_xwindow(Window window)
{
  LNX_GFX_Window *result = 0;
  for each_node(node, LNX_GFX_Window, lnx_gfx_state->windows.first)
  {
    if (node->window == window) 
    {
      result = node;
      break;
    }
  }
  return result;
}

function Key
lnx_keycode_from_keysym(KeySym ks)
{
  Key key = Key_Null;
  switch(ks)
  {
    case XK_Escape:       key = Key_Esc;          break;

    case XK_F1:           key = Key_F1;           break;
    case XK_F2:           key = Key_F2;           break;
    case XK_F3:           key = Key_F3;           break;
    case XK_F4:           key = Key_F4;           break;
    case XK_F5:           key = Key_F5;           break;
    case XK_F6:           key = Key_F6;           break;
    case XK_F7:           key = Key_F7;           break;
    case XK_F8:           key = Key_F8;           break;
    case XK_F9:           key = Key_F9;           break;
    case XK_F10:          key = Key_F10;          break;
    case XK_F11:          key = Key_F11;          break;
    case XK_F12:          key = Key_F12;          break;

    case XK_grave:
    case XK_asciitilde:   key = Key_Tick;         break;

    case XK_0:            key = Key_0;            break;
    case XK_1:            key = Key_1;            break;
    case XK_2:            key = Key_2;            break;
    case XK_3:            key = Key_3;            break;
    case XK_4:            key = Key_4;            break;
    case XK_5:            key = Key_5;            break;
    case XK_6:            key = Key_6;            break;
    case XK_7:            key = Key_7;            break;
    case XK_8:            key = Key_8;            break;
    case XK_9:            key = Key_9;            break;

    case XK_minus:
    case XK_underscore:   key = Key_Minus;        break;
    case XK_equal:
    case XK_plus:         key = Key_Equal;        break;
    case XK_BackSpace:    key = Key_Backspace;    break;
    case XK_Tab:
    case XK_ISO_Left_Tab: key = Key_Tab;          break;

    case XK_q: case XK_Q: key = Key_Q;            break;
    case XK_w: case XK_W: key = Key_W;            break;
    case XK_e: case XK_E: key = Key_E;            break;
    case XK_r: case XK_R: key = Key_R;            break;
    case XK_t: case XK_T: key = Key_T;            break;
    case XK_y: case XK_Y: key = Key_Y;            break;
    case XK_u: case XK_U: key = Key_U;            break;
    case XK_i: case XK_I: key = Key_I;            break;
    case XK_o: case XK_O: key = Key_O;            break;
    case XK_p: case XK_P: key = Key_P;            break;

    case XK_bracketleft:
    case XK_braceleft:    key = Key_LeftBracket;  break;
    case XK_bracketright:
    case XK_braceright:   key = Key_RightBracket; break;
    case XK_backslash:
    case XK_bar:          key = Key_BackSlash;    break;

    case XK_Caps_Lock:    key = Key_CapsLock;     break;

    case XK_a: case XK_A: key = Key_A;            break;
    case XK_s: case XK_S: key = Key_S;            break;
    case XK_d: case XK_D: key = Key_D;            break;
    case XK_f: case XK_F: key = Key_F;            break;
    case XK_g: case XK_G: key = Key_G;            break;
    case XK_h: case XK_H: key = Key_H;            break;
    case XK_j: case XK_J: key = Key_J;            break;
    case XK_k: case XK_K: key = Key_K;            break;
    case XK_l: case XK_L: key = Key_L;            break;

    case XK_semicolon:
    case XK_colon:        key = Key_Semicolon;    break;
    case XK_apostrophe:
    case XK_quotedbl:     key = Key_Quote;        break;
    case XK_Return:
    case XK_KP_Enter:     key = Key_Return;       break;

    case XK_Shift_L:
    case XK_Shift_R:      key = Key_Shift;        break;

    case XK_z: case XK_Z: key = Key_Z;            break;
    case XK_x: case XK_X: key = Key_X;            break;
    case XK_c: case XK_C: key = Key_C;            break;
    case XK_v: case XK_V: key = Key_V;            break;
    case XK_b: case XK_B: key = Key_B;            break;
    case XK_n: case XK_N: key = Key_N;            break;
    case XK_m: case XK_M: key = Key_M;            break;

    case XK_comma:
    case XK_less:         key = Key_Comma;        break;
    case XK_period:
    case XK_greater:      key = Key_Period;       break;
    case XK_slash:
    case XK_question:     key = Key_Slash;        break;

    case XK_Control_L:
    case XK_Control_R:    key = Key_Ctrl;         break;
    case XK_Alt_L:
    case XK_Alt_R:        key = Key_Alt;          break;
    case XK_space:        key = Key_Space;        break;
    case XK_Menu:         key = Key_Menu;         break;

    case XK_Scroll_Lock:  key = Key_ScrollLock;   break;
    case XK_Pause:        key = Key_Pause;        break;
    case XK_Insert:       key = Key_Insert;       break;
    case XK_Home:         key = Key_Home;         break;
    case XK_Prior:        key = Key_PageUp;       break;
    case XK_Delete:       key = Key_Delete;       break;
    case XK_End:          key = Key_End;          break;
    case XK_Next:         key = Key_PageDown;     break;

    case XK_Up:           key = Key_Up;           break;
    case XK_Left:         key = Key_Left;         break;
    case XK_Down:         key = Key_Down;         break;
    case XK_Right:        key = Key_Right;        break;

    default: break;
  }
  return key;
}

function void 
set_cursor_icon(Win_Cursor cursor)
{
  lnx_gfx_state->last_set_cursor = cursor;
}

function b32 
key_is_down(Key key)
{
  b32 result = !!(lnx_gfx_state->key_down_state[key >> 6] & ( 1ull << (key & 0x3F)));
  return result;
}

function Vec2F32 
get_mouse_in_window(Win_Handle win)
{
  if (window_zero(win)) 
  {
    return v2f32(0,0);
  }

  LNX_GFX_Window *w = (LNX_GFX_Window *) win.v[0];
  Vec2F32 result = {0};

  Window root_window = 0;
  Window child_window = 0;
  int root_rel_x = 0;
  int root_rel_y = 0;
  int child_rel_x = 0;
  int child_rel_y = 0;
  unsigned int mask = 0;
  if(XQueryPointer(lnx_gfx_state->display, w->window, &root_window, &child_window, &root_rel_x, &root_rel_y, &child_rel_x, &child_rel_y, &mask))
  {
    result.x = (f32) child_rel_x;
    result.y = (f32) child_rel_y;
  }

  return result;
}

function void
window_set_min_size(Win_Handle win, s32 w, s32 h)
{
  LNX_GFX_Window *lnx_win = (LNX_GFX_Window *) win.v[0];

  XSizeHints *size_hints = XAllocSizeHints();
  size_hints->flags      = PMinSize;
  size_hints->min_width  = w;
  size_hints->min_height = h;
  XSetWMNormalHints(lnx_gfx_state->display, lnx_win->window, size_hints);
  XFree(size_hints);
}

function void 
window_set_background(Win_Handle win, Vec4F32 col)
{
  LNX_GFX_Window *lnx_win = (LNX_GFX_Window *) win.v[0];
  lnx_win->bg_color = col;
}

function Vec2F32
window_get_size(Win_Handle win)
{
  LNX_GFX_Window *w = (LNX_GFX_Window *) win.v[0];
  return v2f32((f32) w->width, (f32) w->height);
}

function void
window_set_move_region(Win_Handle win, f32 regions[Edge_COUNT])
{
  LNX_GFX_Window *w = (LNX_GFX_Window *) win.v[0];
  for each_index(i, Edge_COUNT) {
    w->move_hit_region[i] = regions[i];
  }
}

function Vec4F32  
window_get_move_region(Win_Handle win)
{
  LNX_GFX_Window *w = (LNX_GFX_Window *) win.v[0];
  Vec4F32 region = {0};
  for each_index(i, Edge_COUNT)
  {
    region.xyzw[i] = w->move_hit_region[i];
  }
  return region;
}

function void 
window_sync_frame(Win_Handle win)
{
  LNX_GFX_Window *w = (LNX_GFX_Window *) win.v[0];
  if (w == 0 || !w->sync_pending) 
  {
    return;
  }

  XSyncValue sync_value = {0};
  XSyncIntsToValue(&sync_value,
                   (unsigned int)(w->counter_value & 0xffffffffull),
                   (int)((w->counter_value >> 32) & 0xffffffffull));
  XSyncSetCounter(lnx_gfx_state->display, w->counter_xid, sync_value);
  w->sync_pending = 0;
  XFlush(lnx_gfx_state->display);
}
