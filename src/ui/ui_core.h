#ifndef UI_CORE_H
#define UI_CORE_H

typedef struct UI_Box UI_Box;

typedef struct UI_Key UI_Key;
struct UI_Key
{
  u64 v[1];
};

typedef u32 UI_BoxFlags;
enum
{
  UI_BoxFlag_Clip         = (1u << 0),
  UI_BoxFlag_ScrollX      = (1u << 1),
  UI_BoxFlag_ScrollY      = (1u << 2),
  UI_BoxFlag_AutoActive   = (1u << 3),
  UI_BoxFlag_SmoothWidth  = (1u << 4),
  UI_BoxFlag_SmoothHeight = (1u << 5),
  UI_BoxFlag_SmoothX      = (1u << 6),
  UI_BoxFlag_SmoothY      = (1u << 7),
  UI_BoxFlag_Text         = (1u << 8),
  UI_BoxFlag_Clickable    = (1u << 9),
};

typedef u32 UI_SizeKind;
enum
{
  UI_SizeKind_Fixed,
  UI_SizeKind_Percent,
  UI_SizeKind_Grow,
  UI_SizeKind_Fit,
  UI_SizeKind_COUNT,
};

typedef struct UI_SizeAxis UI_SizeAxis;
struct UI_SizeAxis
{
  UI_SizeKind kind;
  f32 value;
};

typedef union UI_Size UI_Size;
union UI_Size
{
  struct { UI_SizeAxis width, height; };
  UI_SizeAxis axes[2];
};

typedef union UI_Padding UI_Padding;
union UI_Padding
{
  struct { u16 v0, v1, v2, v3; };
  u16 values[Edge_COUNT];
};

typedef void (*UI_BoxDrawProc)(UI_Box *box);

#define X_BOX_CONFIG_FIELDS \
  X(String8,     label) \
  X(UI_BoxFlags, flags) \
  X(Vec2F32,     position) \
  X(Pivot,       pivot) \
  X(Axis2D,      layout) \
  X(UI_Size,     size) \
  X(UI_Padding,  padding) \
  X(f32,         gap) \
  X(f32,         scroll_scale) \
  X(R_Font,      font) \
  X(String8,     text) \
  X(u32,         text_size) \
  X(b32,         text_bold) \
  X(Vec4F32,     text_color) \
  X(f32,         blur_radius) \
  X(Vec4F32,     fill_color) \
  X(Vec4F32,     shadow_color) \
  X(Vec4F32,     border_color) \
  X(f32,         border_thickness) \
  X(Vec4F32,     radii) \
  X(R_Handle,    texture) \

typedef struct UI_Persist UI_Persist;
struct UI_Persist
{
  UI_Key key;
  UI_Box *box;

  UI_Persist *hash_next;
  UI_Persist *hash_prev;

  UI_Persist *next; // persist stack linkage
  u64 last_touched_frame;

  f32 spawn_t;
  f32 last_blur;
  Vec2F32 last_dim;
  Vec2F32 last_pos;
};

// UI_Box; definition
struct UI_Box
{
  UI_Box *parent;
  UI_Box *first;
  UI_Box *last;
  UI_Box *next;
  UI_Box *prev;
  UI_Box *call_prev;
  s64 child_count;
  UI_Persist *persist;

#define X(type, name) type name;
  X_BOX_CONFIG_FIELDS
#undef X

  Vec2F32 final_pos;
  Vec2F32 final_dim;
  Vec2F32 content_dim;
};

typedef struct UI_Config UI_Config;
struct UI_Config
{
#define X(type, name) type name;
  X_BOX_CONFIG_FIELDS
#undef X
};

#define UI_BOX_BUCKET_COUNT 2048

typedef struct UI_FrameInput UI_FrameInput;
struct UI_FrameInput
{
  Vec2F32 resolution;
  b32 mouse_down;
  Vec2F32 mouse_pos;
  f32 delta_time;
};

typedef struct UI_State UI_State;
struct UI_State
{
  Arena  *arena;
  Arena  *build_arena;
  UI_Box *call_list;
  UI_Box *root;
  u64 frame_index;
  
  UI_Persist *persist_buckets[UI_BOX_BUCKET_COUNT];
  UI_Persist *persist_stack;
  UI_Persist *persist_free;
  
  UI_Key hot;
  UI_Key active;

  UI_FrameInput last_input;
  UI_FrameInput curr_input;
};

typedef struct UI_Signal UI_Signal;
struct UI_Signal
{
  b8 active;
  b8 pressed;
  b8 clicked;
  b8 released;
  b8 hovered;
};

global UI_State *ui_state = 0;

////////////////////
// @section: handle functions

function b32 ui_key_zero(UI_Key key);
function b32 ui_key_matches(UI_Key a, UI_Key b);

////////////////////
// @section: ui state functions

function void ui_init(void);

function void ui_begin_layout(UI_FrameInput input);
function void ui_end_layout(R_DrawList *draw_list);

////////////////////
// @section: low level component functions

function UI_Box *ui_open(UI_Box *parent);
function void ui_close(void);
function void ui_box_configure(UI_Config *config);

#define UI(...) for (int _i_ = (ui_open(ui_state->call_list), 0); !_i_ && (ui_box_configure(&(UI_Config) { __VA_ARGS__ }), _i_, 1); _i_ += 1, ui_close())

////////////////////
// @section: layout passes

function void ui_grow_layout_pass(UI_Box *box);
function void ui_position_layout_pass(UI_Box *box);
function void ui_draw_layout_pass(UI_Box *box, R_DrawList *pass);

////////////////////
// @section: persistent data

function UI_Key ui_key_from_string(UI_Key seed, String8 string);
function UI_Persist *ui_get_persist_node(UI_Key key);
function UI_Signal ui_get_signal_from_box(UI_Box *box);

////////////////////
// @section: config utilities

function UI_SizeAxis ui_size_fixed(f32 x);
function UI_SizeAxis ui_size_percent(f32 x);
function UI_SizeAxis ui_size_grow(f32 x);
function UI_SizeAxis ui_size_fit(void);

function UI_Padding ui_padding(f32 v);
function UI_Padding ui_padding_xy(f32 x, f32 y);

function UI_Box *ui_get_current_box(void);

#endif // UI_CORE_H
