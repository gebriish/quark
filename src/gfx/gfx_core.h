#ifndef GFX_CORE_H
#define GFX_CORE_H

//
// @note(gaureesh): graphics half of the base layer. I am pulling
// this out of `base/` because not all targets might need a graphics
// layer. Although, the graphics layer is not tied to a specific
// rendering backend or the render layer. This is meant as a layer that
// provides basic graphical application building blocks.
//

typedef struct GFX_Window GFX_Window;
struct GFX_Window
{
  U64 u64[1];
};

typedef U32 GFX_WindowFlags;
enum
{
  GFX_WindowFlag_None         = 0,
  GFX_WindowFlag_CustomBorder = (1u << 0),
};

typedef U32 GFX_EventKind;
enum
{
  GFX_EventKind_None,
  GFX_EventKind_WindowClose,
  GFX_EventKind_Resize,
  GFX_EventKind_Press,
  GFX_EventKind_Release,
  GFX_EventKind_Scroll,
  GFX_EventKind_MouseMove,
  GFX_EventKind_Text,
  GFX_EventKind_COUNT,
};

typedef U32 GFX_Cursor;
enum
{
  GFX_Cursor_Pointer,
  GFX_Cursor_IBar,
  GFX_Cursor_LeftRight,
  GFX_Cursor_UpDown,
  GFX_Cursor_UpLeft,
  GFX_Cursor_UpRight,
  GFX_Cursor_DownLeft,
  GFX_Cursor_DownRight,
  GFX_Cursor_UpDownLeftRight,
  GFX_Cursor_HandPoint,
  GFX_Cursor_HandGrab,
  GFX_Cursor_Disabled,
  GFX_Cursor_COUNT,
};

typedef U32 GFX_Modifiers;
enum
{
  GFX_Modifier_Ctrl  = 1 << 0,
  GFX_Modifier_Shift = 1 << 1,
  GFX_Modifier_Alt   = 1 << 2,
};

typedef U32 GFX_Key;
enum
{
  GFX_Key_Null,
  GFX_Key_Esc,
  GFX_Key_F1,
  GFX_Key_F2,
  GFX_Key_F3,
  GFX_Key_F4,
  GFX_Key_F5,
  GFX_Key_F6,
  GFX_Key_F7,
  GFX_Key_F8,
  GFX_Key_F9,
  GFX_Key_F10,
  GFX_Key_F11,
  GFX_Key_F12,
  GFX_Key_Tick,
  GFX_Key_0,
  GFX_Key_1,
  GFX_Key_2,
  GFX_Key_3,
  GFX_Key_4,
  GFX_Key_5,
  GFX_Key_6,
  GFX_Key_7,
  GFX_Key_8,
  GFX_Key_9,
  GFX_Key_Minus,
  GFX_Key_Equal,
  GFX_Key_Backspace,
  GFX_Key_Tab,
  GFX_Key_Q,
  GFX_Key_W,
  GFX_Key_E,
  GFX_Key_R,
  GFX_Key_T,
  GFX_Key_Y,
  GFX_Key_U,
  GFX_Key_I,
  GFX_Key_O,
  GFX_Key_P,
  GFX_Key_LeftBracket,
  GFX_Key_RightBracket,
  GFX_Key_BackSlash,
  GFX_Key_CapsLock,
  GFX_Key_A,
  GFX_Key_S,
  GFX_Key_D,
  GFX_Key_F,
  GFX_Key_G,
  GFX_Key_H,
  GFX_Key_J,
  GFX_Key_K,
  GFX_Key_L,
  GFX_Key_Semicolon,
  GFX_Key_Quote,
  GFX_Key_Return,
  GFX_Key_Shift,
  GFX_Key_Z,
  GFX_Key_X,
  GFX_Key_C,
  GFX_Key_V,
  GFX_Key_B,
  GFX_Key_N,
  GFX_Key_M,
  GFX_Key_Comma,
  GFX_Key_Period,
  GFX_Key_Slash,
  GFX_Key_Ctrl,
  GFX_Key_Alt,
  GFX_Key_Space,
  GFX_Key_Menu,
  GFX_Key_ScrollLock,
  GFX_Key_Pause,
  GFX_Key_Insert,
  GFX_Key_Home,
  GFX_Key_PageUp,
  GFX_Key_Delete,
  GFX_Key_End,
  GFX_Key_PageDown,
  GFX_Key_Up,
  GFX_Key_Left,
  GFX_Key_Down,
  GFX_Key_Right,
  GFX_Key_LeftMouseButton,
  GFX_Key_MiddleMouseButton,
  GFX_Key_RightMouseButton,
  GFX_Key_COUNT,
};

typedef U32 GFX_MoveResize;
enum
{
  GFX_MoveResize_TopLeft     = 0,
  GFX_MoveResize_Top         = 1,
  GFX_MoveResize_TopRight    = 2,
  GFX_MoveResize_Right       = 3,
  GFX_MoveResize_BottomRight = 4,
  GFX_MoveResize_Bottom      = 5,
  GFX_MoveResize_BottomLeft  = 6,
  GFX_MoveResize_Left        = 7,
  GFX_MoveResize_Move        = 8,
};

typedef struct GFX_Event GFX_Event;
struct GFX_Event
{
  GFX_EventKind  kind;
  GFX_Event     *prev;
  GFX_Event     *next;
  U32           character;
  GFX_Key       keycode;
  GFX_Modifiers modifiers;
  Vec2F32       size;
  Vec2F32       pos;
  Vec2F32       delta;
  GFX_Window window;
};

typedef struct GFX_EventList GFX_EventList;
struct GFX_EventList
{
  GFX_Event *first;
  GFX_Event *last;
  S64 len;
};

///////////////////////////
// @section: global graphics state

function void gfx_init(void);

function GFX_EventList gfx_get_events(Arena *arena);
function GFX_Event *gfx_event_list_push(Arena *arena, GFX_EventList *list, GFX_EventKind kind);

function B32 gfx_key_is_down(GFX_Key key);
function Vec2F32 gfx_mouse_in_window(GFX_Window win);
function void gfx_set_cursor(GFX_Cursor cursor);

///////////////////////////
// @section: window creation procedures

function B32 gfx_window_zero(GFX_Window w);
function B32 gfx_window_matches(GFX_Window a, GFX_Window b);

function GFX_Window gfx_window_open(String8 title, Vec2F32 resolution, GFX_WindowFlags flags);
function void gfx_window_close(GFX_Window window);

function void gfx_window_set_min_size(GFX_Window win, S32 w, S32 h);
function void gfx_window_set_move_region(GFX_Window win, F32 regions[Edge_COUNT]);
function void gfx_window_set_background(GFX_Window win, Vec4F32 col);

function Vec2F32        gfx_window_size(GFX_Window win);
function GFX_MoveResize gfx_window_moveresize_code(GFX_Window win, Vec2F32 pos, B32 *in_client);
function GFX_Cursor     gfx_cursor_from_moveresize(GFX_MoveResize code);
function Vec4F32        gfx_window_move_region(GFX_Window win);

function void gfx_window_present(GFX_Window win);

#endif // GFX_CORE_H
