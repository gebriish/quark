#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

//
//                   @note(gaureesh):
//
// This is the file that contains all the pre defined widgets.
// Most of the widgets wont be THAT customizable, they arent
// supposed to be. These are supposed to be easy to use drop-in
// widgets where I really dont care about how they look. If I
// want a more custom looking component, I would create it with
// the UI(...) macro found in `ui_core.h`
//
// Also it is imporrant to note that I will not be doing the
// normal capitalization for the macros here. Since they will be
// used a lot ( i think ) I will just make them snake_case for
// ease of use and for it to aesthetically look like functions.
//

////////////////////////////
// @section: macro components

#define ui_layout(l, ...) DeferLoop(ui_begin_layout((UI_FrameInput) {__VA_ARGS__}), ui_end_layout(l))

#define ui_floating(...) \
for (int _i_ = (ui_open(ui_state->root), 0); \
     !_i_ && (ui_box_configure(&(UI_Config) { __VA_ARGS__ }), _i_, 1);\
     _i_ += 1, ui_close())

#define ui_fullscreen(...) ui_floating(.size = { ui_size_percent(1.f), ui_size_percent(1.f) }, __VA_ARGS__)
#define ui_flood(...) UI(.size = {ui_size_grow(1.f), ui_size_grow(1.f) }, __VA_ARGS__ )
#define ui_fit(...) UI(.size = {ui_size_fit(), ui_size_fit()}, __VA_ARGS__ )

#define ui_seperator(along, perp, ...)                    \
do                                                        \
{                                                         \
  UI_Size size = ui_layout_aligned_size((along), (perp)); \
  UI (                                                    \
    .size = size,                                         \
    __VA_ARGS__                                           \
  );                                                      \
} while (0);

////////////////////////////
// @section: function components

function void ui_text(String8 text, R_Font font, u32 size, b32 bold, Vec4F32 color);
function void ui_image(R_Handle tex);
function void ui_image_grown(R_Handle tex);

function b32 ui_button(String8 label,
                       R_Font font,
                       u32 size,
                       Vec4F32 fg,
                       Vec4F32 bg);

////////////////////////////
// @section: components making helpers

function UI_Size ui_layout_aligned_size(UI_SizeAxis along, UI_SizeAxis perp);
function UI_Size ui_grow_proportional(f32 aspect_ratio);

#endif // UI_WIDGETS_H
