#ifndef GFX_CORE_H
#define GFX_CORE_H

//
// @note(gaureesh): graphics half of the base layer. I am pulling
// this out of `base/` because not all targets might need a graphics
// layer. Although, the graphics layer is not tied to a specific
// rendering backend or the render layer. This is meant as a layer that
// provides basic graphical application building blocks.
//

typedef struct Win_Handle Win_Handle;
struct Win_Handle
{
  u64 v[1];
};

typedef u32 Win_Flags;
enum
{
  Win_Flag_CustomBorder = (1u << 0),
};

typedef u32 Win_Event_Kind;
enum
{
  Win_Event_Kind_None,
  Win_Event_Kind_Close,
  Win_Event_Kind_Resize,
  Win_Event_Kind_Press,
  Win_Event_Kind_Release,
  Win_Event_Kind_Scroll,
  Win_Event_Kind_MouseMove,
  Win_Event_Kind_Text,
  Win_Event_Kind_COUNT,
};

typedef u32 Win_Cursor;
enum
{
  Win_Cursor_Pointer,
  Win_Cursor_IBar,
  Win_Cursor_LeftRight,
  Win_Cursor_UpDown,
  Win_Cursor_UpLeft,
  Win_Cursor_UpRight,
  Win_Cursor_DownLeft,
  Win_Cursor_DownRight,
  Win_Cursor_UpDownLeftRight,
  Win_Cursor_HandPoint,
  Win_Cursor_HandGrab,
  Win_Cursor_Disabled,
  Win_Cursor_COUNT,
};

typedef u8 Key_Modifiers;
enum
{
  Key_Modifier_Ctrl  = 1 << 0,
  Key_Modifier_Shift = 1 << 1,
  Key_Modifier_Alt   = 1 << 2,
};

typedef u8 Key;
enum
{
  Key_Null,
  Key_Esc,
  Key_F1,
  Key_F2,
  Key_F3,
  Key_F4,
  Key_F5,
  Key_F6,
  Key_F7,
  Key_F8,
  Key_F9,
  Key_F10,
  Key_F11,
  Key_F12,
  Key_Tick,
  Key_0,
  Key_1,
  Key_2,
  Key_3,
  Key_4,
  Key_5,
  Key_6,
  Key_7,
  Key_8,
  Key_9,
  Key_Minus,
  Key_Equal,
  Key_Backspace,
  Key_Tab,
  Key_Q,
  Key_W,
  Key_E,
  Key_R,
  Key_T,
  Key_Y,
  Key_U,
  Key_I,
  Key_O,
  Key_P,
  Key_LeftBracket,
  Key_RightBracket,
  Key_BackSlash,
  Key_CapsLock,
  Key_A,
  Key_S,
  Key_D,
  Key_F,
  Key_G,
  Key_H,
  Key_J,
  Key_K,
  Key_L,
  Key_Semicolon,
  Key_Quote,
  Key_Return,
  Key_Shift,
  Key_Z,
  Key_X,
  Key_C,
  Key_V,
  Key_B,
  Key_N,
  Key_M,
  Key_Comma,
  Key_Period,
  Key_Slash,
  Key_Ctrl,
  Key_Alt,
  Key_Space,
  Key_Menu,
  Key_ScrollLock,
  Key_Pause,
  Key_Insert,
  Key_Home,
  Key_PageUp,
  Key_Delete,
  Key_End,
  Key_PageDown,
  Key_Up,
  Key_Left,
  Key_Down,
  Key_Right,
  Key_LeftMouseButton,
  Key_MiddleMouseButton,
  Key_RightMouseButton,
  Key_COUNT,
};

typedef struct Key_Map Key_Map;
struct Key_Map
{
  Key keycode;
  Key_Modifiers mods;
};

typedef u32 Win_Region;
enum
{
  Win_Region_TopLeft     = 0,
  Win_Region_Top         = 1,
  Win_Region_TopRight    = 2,
  Win_Region_Right       = 3,
  Win_Region_BottomRight = 4,
  Win_Region_Bottom      = 5,
  Win_Region_BottomLeft  = 6,
  Win_Region_Left        = 7,
  Win_Region_Move        = 8,
};

typedef struct Win_Event Win_Event;
struct Win_Event
{
  Win_Event_Kind kind;
  Win_Event     *prev;
  Win_Event     *next;
  u32           character;
  Key           keycode;
  Key_Modifiers modifiers;
  Vec2F32       size;
  Vec2F32       pos;
  Vec2F32       delta;
  Win_Handle window;
};

typedef struct Win_Event_List Win_Event_List;
struct Win_Event_List
{
  Win_Event *first;
  Win_Event *last;
  s64 len;
};

///////////////////////////
// @section: global graphics state

function void gfx_init(void);

function Win_Event_List get_window_events(Arena *arena);
function Win_Event *window_event_push(Arena *arena, Win_Event_List *list, Win_Event_Kind kind);

function b32 key_is_down(Key key);
function Vec2F32 get_mouse_in_window(Win_Handle win);
function void set_cursor_icon(Win_Cursor cursor);
function b32 keymap_matches(Key_Map a, Key_Map b);

///////////////////////////
// @section: window creation procedures

function b32 window_zero(Win_Handle w);
function b32 window_matches(Win_Handle a, Win_Handle b);

function Win_Handle window_open(String8 title, Vec2F32 resolution, Win_Flags flags);
function void window_close(Win_Handle window);
function void window_sync_frame(Win_Handle win);

function void window_set_min_size(Win_Handle win, s32 w, s32 h);
function void window_set_move_region(Win_Handle win, f32 regions[Edge_COUNT]);
function void window_set_background(Win_Handle win, Vec4F32 col);

function Vec2F32    window_get_size(Win_Handle win);
function Win_Region window_region_from_pixel(Win_Handle win, Vec2F32 pos, b32 *in_client);
function Win_Cursor window_region_to_cursor(Win_Region code);
function Vec4F32    window_get_move_region(Win_Handle win);

#endif // GFX_CORE_H
