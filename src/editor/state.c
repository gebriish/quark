#include "generated/embed_data.h.gen"

global String8 content = {0};

function void
editor_init(Cmd_Line *cmd)
{
  Arena *arena = arena_reserve(Megabytes(16));
  editor_state = arena_push(arena, Editor_State, 1);
  editor_state->arena = arena;

  // loading editor config
  {
    Config config =
    {
      .ui_font   = fnt_alloc(open_sans_ttf_data_string,   Font_Alloc_Flag_NoCopy),
      .code_font = fnt_alloc(inconsolata_ttf_data_string, Font_Alloc_Flag_NoCopy),

      .ui_font_size   = 16,
      .code_font_size = 18,

      .colors =
      {
        [Color_Kind_Normal] = { Color8(0xd3c6aaff), Color8(0x1e2326ff) },
      },
    };

    editor_state->config = config;
  }

  // parseing cli arguments
  String8_List inputs = cmd->inputs;
  for each_node(input_node, String8_Node, inputs.first)
  {
    String8 input = input_node->data;
    File_Info info = file_get_info(input);

    switch (info.kind)
    {
      default: {} break;

      case File_Kind_Directory:
      {
        set_current_working_directory(input);
      } break;
    }
  }

  ScratchScope(scratch, 0, 0)
  {
    String8 cwd = get_current_working_directory(scratch.arena);
    String8 home_dir = get_home_directory();
    String8 title = parent_relative_diretory_from_absolute(home_dir, cwd);
    editor_state->main_window = window_open(title, v2f32(1280, 800), 0);
    render_window_vsync(editor_state->main_window, false);
  }

  content = load_entire_file(arena, S("./src/editor/main.c"));
}

function b32
editor_frame(void)
{
  f32 delta_time = frame_begin();

  Win_Event_List events = get_window_events(frame_arena());
  for each_node(event, Win_Event, events.first)
  {
    switch (event->kind)
    {
      case Win_Event_Kind_Close:
      {
        push_editor_cmd(quit_editor);
      } break;

      case Win_Event_Kind_Press:
      {
        //
        // Process keymaps.
        //

        if (event->keycode == Key_P && event->modifiers == Key_Modifier_Ctrl)
        {
          push_editor_cmd(cmd_palette_widget);
        }

      } break;
    }
  }

  WindowFrameScope(editor_state->main_window)
  {
    Draw_List ui_draw_list = {0};

    ui_layout(&ui_draw_list,
              .resolution = window_get_size(editor_state->main_window),
              .mouse_down = key_is_down(Key_LeftMouseButton),
              .mouse_pos  = get_mouse_in_window(editor_state->main_window),
              .delta_time = delta_time)
    {
      ui_fullscreen(.fill_color = get_color(&editor_state->config, Color_Kind_Normal).bg)
      {
        ui_text(content, 
                editor_state->config.code_font, 
                editor_state->config.code_font_size,
                false,
                get_color(&editor_state->config,
                Color_Kind_Normal).fg);
      }

      if (editor_state->first_command)
      {
        Edit_Cmd_Node *cmd = editor_state->first_command;
        b32 done = cmd->hook(editor_state);
        if (done)
        {
          SLLQueuePop(editor_state->first_command, editor_state->last_command);
          SLLStackPush(editor_state->free_command, cmd);
        }
      }
    }

    submit_draw_list(ui_draw_list);
  }

  frame_end();
  return editor_state->quit;
}

function void 
push_editor_cmd(Edit_Cmd_Hook hook)
{
  if (hook == false) return;

  Edit_Cmd_Node *node = editor_state->free_command;
  if (editor_state->free_command)
  {
    SLLStackPop(editor_state->free_command);
    MemZeroStruct(node);
  }
  else
  {
    node = arena_push(editor_state->arena, Edit_Cmd_Node, 1);
  }
  node->hook = hook;
  SLLQueuePush(editor_state->first_command, editor_state->last_command, node);
}

//////////////////////////
// @section: builtin commands

function b32 
quit_editor(Editor_State *editor)
{
  editor->quit = true;
  return true;
}

function b32 
cmd_palette_widget(Editor_State *editor)
{
  b32 done = false;

  ui_fullscreen(.pivot = Pivot_TopCenter)
  {
    UI(
        .size = { ui_size_percent(0.4f), ui_size_percent(0.6f) },
        .fill_color = Color8(0x00ff00ff),
      ) 
    {

    }
  }

  return done;
}
