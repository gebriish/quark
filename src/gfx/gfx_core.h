#ifndef GFX_CORE_H
#define GFX_CORE_H

#include "../base/base_core.h"
#include "../thirdparty/glad/glad.h"

#define hex_color(x) ByteSwapU32(x)

#define GL_VERSION_MAJOR 3
#define GL_VERSION_MINOR 3

#define MAX_TRIANGLES     4096
#define MAX_VERTEX_COUNT  MAX_TRIANGLES * 3
#define MAX_TEXTURES      16

typedef u32 color8_t; // 0xRRGGBBAA

#define color_r(x) (((x) >> 24) & 0xFF)
#define color_g(x) (((x) >> 16) & 0xFF)
#define color_b(x) (((x) >> 8)  & 0xFF)
#define color_a(x) ((x) & 0xFF)

typedef struct Render_Vertex Render_Vertex;
struct Render_Vertex {
  vec2_f32 position;
  color8_t color;
  vec2_f32 uv;
  vec2_f32 circ_mask;
  f32      tex_id;
};

typedef struct Render_Buffer Render_Buffer;
struct Render_Buffer {
  Render_Vertex *vertices;
  u32 *indices;

  usize vtx_count;
  usize idx_count;
};

typedef u8 Shader_Uniform;
enum {
  Uniform_Textures = 0,
  Uniform_ProjMatrix,
  Uniform_Count
};

typedef struct GFX_State GFX_State;
struct GFX_State {
  u32 vao, vbo, ibo;
  u32 program;
  i32 uniform_loc[Uniform_Count];

  Render_Buffer render_buffer;
  struct { i32 w, h; } viewport;
};

//////////////////////////////////
// ~geb: interface procs

internal_lnk bool gfx_init(Arena *arena, GFX_State *gfx);
internal_lnk void gfx_resize_target(GFX_State *gfx, i32 w, i32 h);

internal_lnk void gfx_begin_frame(GFX_State *gfx, color8_t color);
internal_lnk void gfx_end_frame(GFX_State *gfx);

internal_lnk void gfx_prep(GFX_State *gfx);
internal_lnk void gfx_flush(GFX_State *gfx);

internal_lnk void gfx_push_rect(GFX_State *gfx, vec2_f32 pos, vec2_f32 size, color8_t color);


#endif
