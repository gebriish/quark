function void
ui_text(String8 text, Fnt_Handle font, u32 size, b32 bold, Vec4F32 color)
{
  Vec2F32 txt_size = measure_text_simple(font, text, size, 2);
  UI(
    .size = { ui_size_fixed(txt_size.x), ui_size_fixed(txt_size.y) },
    .text = text,
    .font = font,
    .text_bold = bold,
    .text_size = size,
    .text_color = color,
  );
}

function void
ui_image(Rndr_Handle tex)
{
  Vec2S32 size = texture_size(tex);
  UI(
    .size = { ui_size_fixed((f32) size.x), ui_size_fixed((f32) size.y) },
    .fill_color = Color8(0xffffffff),
    .texture = tex,
  );
}

function void
ui_image_grown(Rndr_Handle tex)
{
  Vec2S32 size = texture_size(tex);
  UI_Size proportional_size = ui_grow_proportional((f32) size.x / (f32) size.y);
  UI(
    .size = proportional_size,
    .fill_color = Color8(0xffffffff),
    .texture = tex,
  );
}

function b32 
ui_button(String8 label, Fnt_Handle font, u32 size, Vec4F32 fg, Vec4F32 bg)
{
  b32 ok = false;
  ui_fit(.label = label,
         .fill_color = bg,
         .flags = UI_BoxFlag_Clickable,
         .padding = ui_padding(5),
         .radii = Radius4(4.f))
  {
    UI_Box *box = ui_get_current_box();
    UI_Signal signal = ui_get_signal_from_box(box);

    if (signal.hovered)
    {
      box->fill_color = vec4_scale(box->fill_color, 1.3f);
    }
    if (signal.pressed)
    {
      box->fill_color = vec4_scale(box->fill_color, .6f);
    }
    if (signal.released)
    {
      ok = true;
    }

    ui_text(label, font, size, false, fg);
  }

  return ok;
}

function UI_Size
ui_layout_aligned_size(UI_SizeAxis along, UI_SizeAxis perp)
{
  UI_Box *box = ui_state->call_list;

  Axis2D layout_axis = box->layout;
  Axis2D layout_perp = axis2d_flip(layout_axis);

  UI_Size size;
  size.axes[layout_axis] = along;
  size.axes[layout_perp] = perp;
  return size;
}

function UI_Size
ui_grow_proportional(f32 aspect_ratio)
{
  UI_Size size = {0};
  UI_Box *parent = ui_state->call_list;

  Axis2D layout_axis = parent->layout;
  Axis2D layout_perp = axis2d_flip(layout_axis);

  size.axes[layout_axis] = ui_size_grow(1.f);
  size.axes[layout_perp] = ui_size_grow(1.f);

  if (parent->persist && aspect_ratio > 0.f)
  {
    Vec2F32 dim = parent->persist->last_dim;

    Vec2F32 padding_sum =
    {
      parent->padding.values[Edge_Left] + parent->padding.values[Edge_Right],
      parent->padding.values[Edge_Top]  + parent->padding.values[Edge_Bottom],
    };

    f32 avail_main  = Max(dim.xy[layout_axis] - padding_sum.xy[layout_axis], 0.f);
    f32 avail_cross = Max(dim.xy[layout_perp] - padding_sum.xy[layout_perp], 0.f);

    if (avail_main > 0.f && avail_cross > 0.f)
    {
      f32 cross = Min(avail_cross, avail_main / aspect_ratio);
      f32 main  = cross * aspect_ratio;

      size.axes[layout_axis] = ui_size_fixed(main);
      size.axes[layout_perp] = ui_size_fixed(cross);
    }
  }

  return size;
}

