#include "generated/generated_builtin_font_data.h.gen"

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
    quark_state->root_window = gfx_window_open(S("quark"), V2(1280, 800), 0);
    gfx_window_set_min_size(quark_state->root_window, 120,120);
    gfx_window_set_background(quark_state->root_window, Color8(0x131314ff));
    r_window_set_vsync(quark_state->root_window, false);
  }

  // @todo(gaureesh): load from a config file
  {
    Q_Config config = 
    {
      .ui_font = r_fnt_alloc(open_sans_ttf_data_string, R_FontAllocFlag_NoCopy),
      .ui_font_size = 15,

      .code_font = r_fnt_alloc(inconsolata_ttf_data_string, R_FontAllocFlag_NoCopy),
      .code_font_size = 16,

      .colors =
      {
        [Q_ColorKind_Normal]    = { .fg = Color8(0xD3C6AAFF), .bg = Color8(0x1E2326FF) },
        [Q_ColorKind_Border]    = { .fg = Color8(0x00000000), .bg = Color8(0x475258FF) },
        [Q_ColorKind_Cursor]    = { .fg = Color8(0x2D353BFF), .bg = Color8(0xA7C080FF) },
        [Q_ColorKind_Floating]  = { .fg = Color8(0x7A8478FF), .bg = Color8(0x272E33FF) },
        [Q_ColorKind_TextBox]   = { .fg = Color8(0x7A8478FF), .bg = Color8(0x1E2326FF) },
      },
    };
    quark_state->config = config;
  }

  q_open_buffer_from_path(S("./src/quark/quark_core.c"));
}


function B32
q_frame(void)
{
  //---------------------------------------------------------------------------//

  GFX_EventList events = gfx_get_events(r_frame_arena());
  for EachNode(ev, GFX_Event, events.first) switch (ev->kind)
  {
    case GFX_EventKind_WindowClose:
    {
      if (gfx_window_matches(quark_state->root_window, ev->window))
      {
        q_push_command(q_command_quit);
      }
      else
      {
        gfx_window_close(ev->window);
      }
    } break;

    case GFX_EventKind_Press:
    {

      if (ev->keycode == GFX_Key_Semicolon && ev->modifiers == (GFX_Modifier_Shift))
      {
        q_push_command(q_command_command_mode);
      }
      if (ev->keycode == GFX_Key_Esc)
      {
        quark_state->mode = Q_Mode_Normal;
      }

    } break;
  }

  Q_Command *cmd = quark_state->first_command;
  if (cmd && cmd->hook(quark_state))
  {
    SLLQueuePop(quark_state->first_command, quark_state->last_command);
  }

  //---------------------------------------------------------------------------//

  F32 delta_time = r_frame_begin();

  WindowFrameScope(quark_state->root_window)
  {
    Vec2F32 window_size = gfx_window_size(quark_state->root_window);
    R_DrawList draw_list = {0};

    ui_layout(&draw_list, 
              .resolution = window_size, 
              .mouse_down = false,
              .mouse_pos  = gfx_mouse_in_window(quark_state->root_window),
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
            Color8(0xDBBB7EFF)
          );

          ui_text(
            S("•"),
            quark_state->config.ui_font,
            quark_state->config.ui_font_size,
            true,
            Color8(0xDBBB7EFF)
          );

          ui_text(
            quark_state->first_buffer->path,
            quark_state->config.ui_font,
            quark_state->config.ui_font_size,
            false,
            Color8(0x7A8478FF)
          );
        }
      }

      if (quark_state->mode == Q_Mode_Command)
        ui_fullscreen(.padding = ui_padding(32.f),
                      .layout = Axis2D_Vertical,
                      .fill_color = Color_White,
                      .blur_radius = 5.f,
                      .pivot = Pivot_TopCenter,)
      {
        UI(.label = S("command_palette"),
           .flags =  UI_BoxFlag_SmoothY | UI_BoxFlag_SmoothWidth,
           .size = { ui_size_percent(.7f), ui_size_percent(.7f) },
           .fill_color = q_color(Q_ColorKind_Floating).bg,
           .shadow_color = Color8(0x00000040),
           .radii = Radius4(6.f),
           .border_color = q_color(Q_ColorKind_Border).bg,
           .padding = ui_padding(4),
           .layout = Axis2D_Vertical)
        {
          UI(.size = {ui_size_grow(1.f), ui_size_fit()},
             .padding = ui_padding(4),
             .flags = UI_BoxFlag_Clip | UI_BoxFlag_Text,
             .label = S("command_form"),
           .radii = Radius4(2.f),
             .border_color = q_color(Q_ColorKind_Border).fg,
             .fill_color = q_color(Q_ColorKind_TextBox).bg)
          {
            ui_text(S("Run Command..."),
                    quark_state->config.ui_font,
                    quark_state->config.ui_font_size,
                    false,
                    q_color(Q_ColorKind_TextBox).fg);
          }
        }
      }


    }

    r_window_submit_frame(draw_list);
  }
  r_frame_end();

  return quark_state->quit;
}

/////////////////////////////////
// @section: builtin command hooks

function B32 
q_command_quit(Q_State *quark)
{
  quark->quit = true;
  return true;
}

function B32 
q_command_command_mode(Q_State *quark)
{
  quark->mode = Q_Mode_Command;
  return true;
}

/////////////////////////////////
// @section: helper utils

function Q_Color 
q_color(Q_ColorKind kind)
{
  return quark_state->config.colors[kind];
}

