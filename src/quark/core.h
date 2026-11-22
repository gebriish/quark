#ifndef QUARK_CORE_H
#define QUARK_CORE_H

/////////////////////////////////
// @section: types

typedef u32 Edit_Mode;
enum
{
  Edit_Mode_Normal,
  Edit_Mode_Insert,
  Edit_Mode_Command,
  Edit_Mode_COUNT,
};

read_only global String8 edit_mode_string[Edit_Mode_COUNT] =
{
  [Edit_Mode_Normal]  = S("normal"),
  [Edit_Mode_Insert]  = S("insert"),
  [Edit_Mode_Command] = S("command"),
};

typedef u32 Q_ActionKind;
enum
{
  Q_ActionKind_Nil,
  Q_ActionKind_Quit,
  Q_ActionKind_COUNT,
};

typedef u32 Q_ColorKind;
enum
{
  Q_ColorKind_Normal,
  Q_ColorKind_Border,
  Q_ColorKind_Cursor,
  Q_ColorKind_Floating,
  Q_ColorKind_TextBox,
  Q_ColorKind_FileName,
  Q_ColorKind_COUNT,
};

typedef struct Q_Color Q_Color;
struct Q_Color
{
  Vec4F32 fg;
  Vec4F32 bg;
};

typedef struct Q_Config Q_Config;
struct Q_Config
{
  Fnt_Handle ui_font;
  Fnt_Handle code_font;
  u32 ui_font_size;
  u32 code_font_size;
  Q_Color colors[Q_ColorKind_COUNT];
};

typedef struct Q_Buffer Q_Buffer;
struct Q_Buffer
{
  Q_Buffer *hash_next;
  Q_Buffer *hash_prev;

  Q_Buffer *next;
  Q_Buffer *prev;

  Arena *arena;
  String8 path;
  String8 name;
  File_Info file_info;

  String8 original;
};

typedef struct Q_BufferHashSlot Q_BufferHashSlot;
struct Q_BufferHashSlot
{
  Q_Buffer *first;
  Q_Buffer *last;
};

typedef u32 Q_PanelKind;
enum
{
  Q_PanelKind_Editor,
  Q_PanelKind_VSPlit,
  Q_PanelKind_HSPlit,
};

typedef struct Q_Panel Q_Panel;
struct Q_Panel
{
  Q_Panel *parent;

  union
  {
    struct // Buffer
    {
      Q_Buffer *buffer;
    };

    struct // VSplit | HSplit
    {
      Q_Panel *first;
      Q_Panel *second;
    };
  };
};

typedef struct Q_PanelTree Q_PanelTree;
struct Q_PanelTree
{
  Q_Panel root;
  Q_Panel *active;
};

typedef struct Q_Command Q_Command;

typedef struct Q_State Q_State;
struct Q_State
{
  Arena *arena;
  Arena *modal_arena;

  Edit_Mode mode;
  Win_Handle root_window;

  Q_BufferHashSlot buffer_table[1024];
  Q_Buffer *first_buffer;
  Q_Buffer *last_buffer;
  s64 buffer_count;

  Q_PanelTree panel_tree;

  Q_Command *first_command;
  Q_Command *last_command;
  Q_Command *free_command_nodes;

  Q_Config config;
  b32 quit;
};

typedef b32 (*Q_CommandHook)(Q_State *quark);
struct Q_Command
{
  Q_CommandHook hook;
  Q_Command *next;
};

global Q_State *quark_state = 0;

/////////////////////////////////
// @section: state management functions

function void q_init(void);
function b32 q_frame(void);
function f32 q_frame_time(void);

function void q_push_command(Q_CommandHook hook);

/////////////////////////////////
// @section: builtin command hooks

function b32 q_command_quit(Q_State *quark);
function b32 q_command_command_mode(Q_State *quark);

/////////////////////////////////
// @section: helper utils

function Q_Color q_color(Q_ColorKind kind);

#endif // QUARK_CORE_H
