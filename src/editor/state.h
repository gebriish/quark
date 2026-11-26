#ifndef EDITOR_CORE_H
#define EDITOR_CORE_H

//////////////////////////
// @section: state types

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

typedef struct Editor_State Editor_State;

typedef b32 (*Edit_Cmd_Hook)(Editor_State *state);

typedef struct Edit_Cmd_Node Edit_Cmd_Node;
struct Edit_Cmd_Node
{
  Edit_Cmd_Hook hook;
  Edit_Cmd_Node *next;
};

typedef struct Key_Map Key_Map;
struct Key_Map
{
  Key keycode;
  Key_Modifiers modifiers;
  Edit_Cmd_Hook cmd;
};

struct Editor_State
{
  Arena     *arena;
  Edit_Mode  mode;

  Win_Handle main_window;
  Config     config;

  Edit_Cmd_Node *first_command;
  Edit_Cmd_Node *last_command;
  Edit_Cmd_Node *free_command;

  b32 quit;
};

global Editor_State *editor_state = 0;

//////////////////////////
// @section: state procedures

function void editor_init(Cmd_Line *cmd);
function b32 editor_frame(void);
function void push_editor_cmd(Edit_Cmd_Hook hook);

//////////////////////////
// @section: builtin commands

function b32 quit_editor(Editor_State *editor);
function b32 cmd_palette_widget(Editor_State *editor);

#endif // EDITOR_CORE_H
