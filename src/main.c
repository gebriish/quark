////////////////////////////////
// ~geb: .h Includes
#include "base/base_inc.h"
#include "gfx/gfx_inc.h"
#include "quark/quark_inc.h"

////////////////////////////////
// ~geb: .c Includes
#include "base/base_inc.c"
#include "gfx/gfx_inc.c"
#include "quark/quark_inc.c"

#define TARGET_FRAME_RATE 120
#define TARGET_FRAME_TIME (1000.0/TARGET_FRAME_RATE)

typedef struct Text_Cursor {
  f32 x;
  f32 y;
} Text_Cursor;

internal_lnk Text_Cursor
push_text(GFX_State *gfx, String8 text, Font_Atlas *atlas, u32 atlas_id,
          f32 x, f32 y, u32 tab_width, color8_t color)
{
  if (text.len == 0) {
    return (Text_Cursor){x, y};
  }

  Font_Metrics *metrics = &atlas->metrics;
  f32 line_height = font_metrics_line_height(metrics);
  f32 ascent      = font_metrics_baseline_offset(metrics);

  f32 tab_advance = line_height * 0.5f * (f32)tab_width; // keeps tab roughly consistent

  Glyph_Info error_glyph;
  bool error_ok = glyph_map_get(atlas->code_to_glyph, '?', &error_glyph);

  f32 cursor_x = x;
  f32 cursor_y = y;

  f32 atlas_width_f  = (f32)atlas->dim.x;
  f32 atlas_height_f = (f32)atlas->dim.y;

  str8_foreach(text, itr, i) {
    u32 codepoint = itr.codepoint;

    switch (codepoint) {
      case '\n': {
        cursor_x = x;
        cursor_y += line_height;
        continue;
      }
      case '\t': {
        cursor_x += tab_advance;
        continue;
      }
      case '\r': {
        cursor_x = x;
        continue;
      }
    }

    Glyph_Info glyph;
    bool ok = glyph_map_get(atlas->code_to_glyph, codepoint, &glyph);
    color8_t glyph_col = color;

    if (!ok && error_ok) {
      glyph = error_glyph;
      glyph_col = 0xff0000ff;
    }

    if (rune_is_space(codepoint)) {
      cursor_x += (f32)glyph.advance;
      continue;
    }

    f32 glyph_x = cursor_x + (f32)glyph.bearing.x;
    f32 glyph_y = cursor_y + ascent - (f32)glyph.bearing.y;
    f32 glyph_width  = (f32)glyph.size.x;
    f32 glyph_height = (f32)glyph.size.y;

    if (glyph_width > 0 && glyph_height > 0) {
      f32 u0 = (f32)glyph.position.x / atlas_width_f;
      f32 v0 = (f32)glyph.position.y / atlas_height_f;
      f32 u1 = (f32)(glyph.position.x + glyph.size.x) / atlas_width_f;
      f32 v1 = (f32)(glyph.position.y + glyph.size.y) / atlas_height_f;
  
      bool cond = codepoint == '{' || codepoint == '}';
      if (cond) {
        push_rect_rounded(
          gfx,
          .position = (vec2_f32){cursor_x, cursor_y},
          .size     = (vec2_f32){glyph.advance, line_height},
          .uv       = (vec4_f32){0,0,1,1},
          .color    = 0x00ff00ff,
          .tex_id   = WHITE_TEXTURE,
          .radii    = {32,32,32,32},
        );
      }

      push_rect(gfx,
        .position = (vec2_f32){glyph_x, glyph_y},
        .size     = (vec2_f32){glyph_width, glyph_height},
        .uv       = (vec4_f32){u0, v0, u1, v1},
        .color    =  cond ? 0x131313ff : glyph_col,
        .tex_id   = (u8)atlas_id
      );
    }

    cursor_x += (f32)glyph.advance;
  }

  return (Text_Cursor){cursor_x, cursor_y};
}


internal_lnk void
resize_callback(RGFW_window *window, i32 w, i32 h)
{
  GFX_State *gfx = (GFX_State *)RGFW_window_getUserPtr(window);
  gfx_resize_target(gfx, w, h);
}

int main(void)
{
  // Initialize Quark context
  Quark_Context ctx;
  quark_new(&ctx);

  // Configure OpenGL hints
  RGFW_glHints *hints = RGFW_getGlobalHints_OpenGL();
  hints->major = 3;
  hints->minor = 3;
  RGFW_setGlobalHints_OpenGL(hints);

  // Create window
  RGFW_window *window = RGFW_createWindow(
    "quark",
    0, 0, 1000, 625,
    RGFW_windowCenter | RGFW_windowOpenGL | RGFW_windowHide
  );
  if (window == NULL) {
    LogError("Failed to create RGFW window\n");
    return -1;
  }

  RGFW_window_makeCurrentContext_OpenGL(window);
  RGFW_window_swapInterval_OpenGL(window, 0);

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL)) {
    LogError("Failed to initialize GLAD\n");
    return -1;
  }

  // Initialize graphics state
  GFX_State gfx;
  if (!gfx_init(ctx.persist_arena, ctx.transient_arena, &gfx)) {
    LogError("Failed to initialize GFX\n");
    return -1;
  }
  gfx_resize_target(&gfx, 1000, 625);

  // Set up window callbacks
  RGFW_window_setUserPtr(window, (void *)(&gfx));
  RGFW_setWindowResizedCallback(resize_callback);

  RGFW_window_show(window);

  // Initialize font system
  font_init(ctx.persist_arena, str8_lit("./res/fonts/jetbrains_mono.ttf"));

  // Generate font atlas
  Temp tmp = temp_begin(ctx.transient_arena);
  Font_Atlas atlas = font_generate_atlas(
    ctx.persist_arena,
    ctx.transient_arena,
    18,
    str8_lit(FONT_CHARSET)
  );

  // Upload atlas texture
  Texture_Data data = {
    .data = atlas.image,
    .width = atlas.dim.x,
    .height = atlas.dim.y,
    .channels = 1,
  };
  u32 atlas_id = gfx_texture_upload(&gfx, data, TextureKind_GreyScale);
  atlas.image = NULL;
  temp_end(tmp);

  // Print memory usage
  arena_print_usage(ctx.persist_arena, "persist_arena");
  arena_print_usage(ctx.transient_arena, "transient_arena");

  Time_Stamp last_time = time_now();
  while (!RGFW_window_shouldClose(window)) {
    Time_Stamp frame_start = time_now();
    Time_Duration diff = time_diff(last_time, frame_start);
    last_time = frame_start;
    f64 delta_time = diff.seconds;

    RGFW_pollEvents();
    arena_clear(ctx.transient_arena);

    gfx_begin_frame(&gfx, 0x131313ff);

    push_text(
      &gfx,
      str8_lit(
        "int main() {\n"
        "    float π = 3.14159f; // Greek letter pi\n"
        "    int y = 42;\n"
        "    const char *msg = \"¡Hola! Grüß Gott. Здравствуйте. Καλημέρα. こんにちは\";\n"
        "    printf(\"Message: %s\\n\", msg);\n"
        "\n"
        "    // Test all kinds of characters\n"
        "    // ASCII: !@#$%^&*()_+-=[]{}|;:'\",.<>/?\n"
        "    // Latin: ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÑÒÓÔÕÖØÙÚÛÜÝÞß\n"
        "    // Greek: ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ αβγδεζηθικλμνξοπρστυφχψω\n"
        "    // Cyrillic: АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ абвгдеёжзийклмнопрстуфхцчшщъыьэюя\n"
        "\n"
        "    for (int i = 0; i < y; ++i) {\n"
        "        π += sinf(i * 0.1f);\n"
        "        printf(\"i=%d π=%.3f -> %c\\n\", i, π, 'λ');\n"
        "    }\n"
        "\n"
        "    // Mathematical and symbolic test\n"
        "    // ≈ ≠ ≤ ≥ ± × ÷ ∑ ∏ ∞ √ ∫ ° Ω µ α β γ δ ε φ ψ ω\n"
        "\n"
        "    // Japanese kana test\n"
        "    // あいうえお アイウエオ カキクケコ ん ツ シ\n"
        "\n"
        "    return 0;\n"
        "}\n"
      ),
      &atlas,
      atlas_id,
      0, 0,
      4,
      0x99806aff
    );

    gfx_end_frame(&gfx);
    RGFW_window_swapBuffers_OpenGL(window);
  }

  RGFW_window_close(window);
  quark_delete(&ctx);

  return 0;
}
