#ifndef GFX_CORE_H
#define GFX_CORE_H


#include "../base/base_core.h"
#include "../base/base_string.h"

#include "../thirdparty/glad/glad.h"
#include "../thirdparty/glfw/include/GLFW/glfw3.h"

#include "gfx_font.h"

#define hex_color(x) ByteSwapU32(x)

#define GL_VERSION_MAJOR 3
#define GL_VERSION_MINOR 3

#define MAX_TRIANGLES     1024
#define MAX_VERTEX_COUNT  MAX_TRIANGLES * 3
#define MAX_TEXTURES      16

#define WHITE_TEXTURE     0

typedef u32 color8_t; // 0xRRGGBBAA

#define color_r(x) (((x) >> 24) & 0xFF)
#define color_g(x) (((x) >> 16) & 0xFF)
#define color_b(x) (((x) >> 8)  & 0xFF)
#define color_a(x) ((x) & 0xFF)

typedef struct Render_Vertex Render_Vertex;
struct Render_Vertex {
  vec2_f32 position;
  color8_t color;
  vec2_u16 uv;
  vec2_i8  circ_mask;
  u8       tex_id;
};

typedef struct Render_Buffer Render_Buffer;
struct Render_Buffer {
  Render_Vertex *vertices;
  u16 *indices;

  u32 vtx_count;
  u32 idx_count;
};

Enum(Shader_Uniform, u8) {
  Uniform_Textures = 0,
  Uniform_ProjMatrix,
  Uniform_Count
};

typedef struct Texture Texture;
struct Texture {
  u32 gl_id;
  u32 next; // freelist pointer to next ~internal id~
};

typedef struct Texture_Data Texture_Data;
struct Texture_Data {
  u8 *data;
  i32 width, height;
  i32 channels;
};

Enum(Texture_Kind, u8) {
  TextureKind_Normal = 0,
  TextureKind_GreyScale,
  TextureKind_Count
};

typedef struct GFX_State GFX_State;
struct GFX_State {
	Arena *persist_arena;

  i32 uniform_loc[Uniform_Count];
  struct { i32 w, h; } viewport;

  // Geometry
  Render_Buffer render_buffer;
  u32 vao, vbo, ibo;
  u32 program;

  // Texture
  Texture texture_slots[MAX_TEXTURES];
  u32 texture_count; 
  u32 texture_freelist;

  GLFWwindow *glfw_window;

	Font_Atlas font_atlas;
};

//////////////////////////////////
// ~geb: interface parameters

typedef struct {
  f32 top_left;
  f32 top_right;
  f32 bottom_right;
  f32 bottom_left;
} Rect_Radii;

#define rect_radius(x) (Rect_Radii) {x,x,x,x}

typedef struct Rect_Params Rect_Params;
struct Rect_Params {
  vec2_f32 position;
  vec2_f32 size;
  color8_t color;
  Rect_Radii radii;
  vec4_f32 uv;
  u8 tex_id;
};

typedef struct Text_Metrics Text_Metrics;
struct Text_Metrics {
	vec2_f32 size;
	vec2_f32 min;
	vec2_f32 max;
};

typedef u32 Text_Align;
enum {
	Text_Align_Left, 
	Text_Align_Center,
	Text_Align_Right, 

	Text_Align_Top = Text_Align_Left,
	Text_Align_Middle = Text_Align_Center,
	Text_Align_Bottom = Text_Align_Right,
};

//////////////////////////////////
// ~geb: interface procs

internal bool gfx_init(GFX_State *gfx);
internal void gfx_deinit();

internal bool gfx_window_open();
internal vec2_i32 gfx_window_size(); 

internal void gfx_resize_target(i32 w, i32 h);

internal void gfx_begin_frame(color8_t color);
internal void gfx_end_frame();

internal void gfx_prep();
internal void gfx_flush();

internal u32  gfx_texture_upload(Texture_Data data, Texture_Kind type);
internal bool gfx_texture_update(u32 id, i32 w, i32 h, i32 channels, u8 *data);
internal bool gfx_texture_unload(u32 id);

internal void gfx_push_rect(Rect_Params *params);
internal void gfx_push_rect_rounded(Rect_Params *params);

#define push_rect(...) gfx_push_rect(&(Rect_Params) {__VA_ARGS__})
#define push_rect_rounded(...) gfx_push_rect_rounded(&(Rect_Params) {__VA_ARGS__})

internal void gfx_push_text(String8 text, vec2_f32 pos, color8_t color);
internal void gfx_push_text_aligned(String8 text, vec2_f32 pos, color8_t color, Text_Align h_align, Text_Align v_align);


#endif
