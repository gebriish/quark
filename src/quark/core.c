#include "generated/embed_data.h.gen"

function void
q_init(void)
{
  Arena *arena = arena_reserve(Megabytes(4));

  // state init
  {
    quark_state = arena_push(arena, Q_State, 1);
    quark_state->arena = arena;
    quark_state->modal_arena = arena_reserve(Megabytes(32));
  }

  // creating root window
  {
    quark_state->root_window = window_open(S("quark"), v2f32(1280, 800), 0);
    window_set_min_size(quark_state->root_window, 120,120);
    window_set_background(quark_state->root_window, Color8(0xff00ffff));
    render_window_vsync(quark_state->root_window, false);
  }

  // @todo(gaureesh): load from a config file
  {
    Q_Config config = 
    {
      .ui_font = fnt_alloc(open_sans_ttf_data_string, Font_Alloc_Flag_NoCopy),
      .ui_font_size = 14,

      .code_font = fnt_alloc(inconsolata_ttf_data_string, Font_Alloc_Flag_NoCopy),
      .code_font_size = 16,

      .colors = 
      {
        [Q_ColorKind_Normal]    = { .fg = Color8(0xD3C6AAFF), .bg = Color8(0x1E2326FF) },
        [Q_ColorKind_Border]    = { .fg = Color8(0xA7C080FF), .bg = Color8(0x00000000) },
        [Q_ColorKind_Cursor]    = { .fg = Color8(0x2D353BFF), .bg = Color8(0xA7C080FF) },
        [Q_ColorKind_Floating]  = { .fg = Color8(0x7A8478FF), .bg = Color8(0x272E33FF) },
        [Q_ColorKind_FileName]  = { .fg = Color8(0xDBBB7EFF), .bg = Color8(0x7A8478FF) },
        [Q_ColorKind_TextBox]   = { .fg = Color8(0x7A8478FF), .bg = Color8(0x1E2326FF) },
      },
    };
    quark_state->config = config;
  }

  open_buffer_from_path(S("./src/quark/buffer.c"));
}


function b32
q_frame(void)
{
  //---------------------------------------------------------------------------//

  Win_Event_List events = get_window_events(frame_arena());
  for each_node(ev, Win_Event, events.first) switch (ev->kind)
  {
    case Win_Event_Kind_Close:
    {
      if (window_matches(quark_state->root_window, ev->window))
      {
        q_push_command(q_command_quit);
      }
      else
      {
        window_close(ev->window);
      }
    } break;

    case Win_Event_Kind_Press:
    {
      Key_Map cmd_key = {
        Key_Semicolon,
        Key_Modifier_Shift,
      };
      Key_Map esc_key = {
        Key_Esc,
        0,
      };

      Key_Map input = {
        .keycode = ev->keycode,
        .mods = ev->modifiers,
      };
      
      if (keymap_matches(input, cmd_key))
      {
        quark_state->mode = Edit_Mode_Command;
      }
      if (keymap_matches(input, esc_key))
      {
        quark_state->mode = Edit_Mode_Normal;
      }
    } break;
  }

  if (quark_state->first_command)
  {
    quark_state->first_command->hook(quark_state);
    SLLQueuePop(quark_state->first_command, quark_state->last_command);
  }

  //---------------------------------------------------------------------------//

  f32 delta_time = frame_begin();

  WindowFrameScope(quark_state->root_window)
  {
    Vec2F32 window_size = window_get_size(quark_state->root_window);
    Draw_List draw_list = {0};

    ui_layout(&draw_list, 
              .resolution = window_size, 
              .mouse_down = key_is_down(Key_LeftMouseButton),
              .mouse_pos  = get_mouse_in_window(quark_state->root_window),
              .delta_time = delta_time)
    {
      ui_fullscreen(.fill_color = q_color(Q_ColorKind_Normal).bg,
                    .layout = Axis2D_Vertical)
      {
        ui_flood()
        {
          ui_text(
            quark_state->first_buffer->original,
            quark_state->config.code_font,
            quark_state->config.code_font_size,
            false,
            q_color(Q_ColorKind_Normal).fg
          );
        }

        UI(.size = { ui_size_grow(1.f), ui_size_fit()},
           .padding = ui_padding(4),
           .shadow_color = Color8(0x00000034),
           .gap = 8,
           .pivot = Pivot_MidLeft,
           .fill_color = q_color(Q_ColorKind_Floating).bg)
        {

          ui_text(
            quark_state->first_buffer->name,
            quark_state->config.ui_font,
            quark_state->config.ui_font_size,
            true,
            q_color(Q_ColorKind_FileName).fg
          );

          ui_text(
            S("•"),
            quark_state->config.ui_font,
            quark_state->config.ui_font_size,
            true,
            q_color(Q_ColorKind_FileName).fg
          );

          ui_text(
            quark_state->first_buffer->path,
            quark_state->config.ui_font,
            quark_state->config.ui_font_size,
            false,
            q_color(Q_ColorKind_Floating).fg
          );
        }
      }

      if (quark_state->mode == Edit_Mode_Command)
        ui_fullscreen(.padding = ui_padding(0.f),
                      .layout = Axis2D_Vertical,
                      .pivot = Pivot_LowRight)
      {
        UI(.label = S("command_palette"),
           .flags = UI_BoxFlag_SmoothHeight,
           .size = { ui_size_percent(.3f), ui_size_percent(.6f) },
           .radii = { 6, 0, 0, 0 },
           .pivot = Pivot_TopRight,
           .fill_color = q_color(Q_ColorKind_Floating).bg,
           .shadow_color = Color8(0x00000022),
           .border_color = q_color(Q_ColorKind_Border).bg,
           .layout = Axis2D_Vertical)
        {
          ui_flood(.flags = UI_BoxFlag_Clip,
                   .gap = 5,
                   .padding = { .values = {5,5,0,5} },
                   .layout = Axis2D_Vertical)
          {
            for each_index(i, 10)
            {
              String8 label = str8_format(frame_arena(), "button %ld", i);
              if (ui_button(label, quark_state->config.code_font, quark_state->config.code_font_size, q_color(Q_ColorKind_Floating).fg, q_color(Q_ColorKind_Floating).bg))
              {
              }
            }
          }

          UI(.padding = ui_padding(8),
             .gap = 8.f,
             .pivot = Pivot_MidLeft,
             .size = { ui_size_grow(1.f), ui_size_fit() })
          {
            UI(.size = {ui_size_grow(1.f), ui_size_fit()},
               .padding = ui_padding(4),
               .flags = UI_BoxFlag_Clip | UI_BoxFlag_Text | UI_BoxFlag_AutoActive,
               .label = S("command_form"),
               .radii = Radius4(4),
               .border_color = q_color(Q_ColorKind_Border).bg,
               .fill_color = q_color(Q_ColorKind_TextBox).bg)
            {
              UI_Box *this_box = ui_get_current_box();
              UI_Signal sig = ui_get_signal_from_box(this_box);
              if (sig.active)
              {
                this_box->border_color = q_color(Q_ColorKind_Border).fg;
              }
              ui_text(S("Run Command..."),
                      quark_state->config.ui_font,
                      quark_state->config.ui_font_size,
                      false,
                      q_color(Q_ColorKind_TextBox).fg);

            }
          }
        }
      }


    }

    submit_draw_list(draw_list);
  }
  frame_end();
  return quark_state->quit;
}

function void 
q_push_command(Q_CommandHook hook)
{
  Q_Command *node = quark_state->free_command_nodes;
  if (quark_state->free_command_nodes)
  {
    SLLStackPop(quark_state->free_command_nodes);
    MemZeroStruct(node);
  }
  else
  {
    node = arena_push(quark_state->arena, Q_Command, 1);
  }

  node->hook = hook;
  SLLQueuePush(quark_state->first_command, quark_state->last_command, node);
}

/////////////////////////////////
// @section: builtin command hooks

function b32 
q_command_quit(Q_State *quark)
{
  quark->quit = true;
  return true;
}

/////////////////////////////////
// @section: helper utils

function Q_Color 
q_color(Q_ColorKind kind)
{
  return quark_state->config.colors[kind];
}

