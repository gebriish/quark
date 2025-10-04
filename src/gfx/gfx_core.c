#include "gfx_core.h"

#include "../thirdparty/glad/glad.c"

#define VTX_SIZE sizeof(Render_Vertex)

const char* VERTEX_SHADER = 
  "#version 330 core\n"

  "layout (location = 0) in vec2 Position;"
  "layout (location = 1) in vec4 Color;"
  "layout (location = 2) in vec2 UV;"
  "layout (location = 3) in vec2 CircCoords;"
  "layout (location = 4) in float TexID;"

  "uniform mat4 ProjMatrix;"

  "out vec2 Frag_UV;"
  "out vec2 Frag_CircCoords;"
  "out vec4 Frag_Color;"
  "out float Frag_TexID;"

  "void main() {"
  "    Frag_UV = UV;"
  "    Frag_Color = Color;"
  "    Frag_CircCoords = CircCoords;"
  "    Frag_TexID = TexID;"

  "    gl_Position = ProjMatrix * vec4(Position, 0, 1);"
  "}";

const char* FRAGMENT_SHADER = 
  "#version 330 core\n"
  "in vec2 Frag_UV;"
  "in vec2 Frag_CircCoords;"
  "in vec4 Frag_Color;"
  "in float Frag_TexID;"

  "uniform sampler2D Textures[16];"

  "layout (location = 0) out vec4 Out_Color;"

  "void main() {"
  "    int tex_id = int(Frag_TexID);"
  "    float dist = length(Frag_CircCoords) - 1.0;"
  "    float edge_width = fwidth(dist) * 0.5;"
  "    float alpha = 1.0 - smoothstep(-edge_width, edge_width, dist);"
  //"    vec4 base = frag_color * texture(textures[tex_id], frag_uv);"
  "    vec4 base = Frag_Color;"
  "    Out_Color = vec4(base.rgb, base.a * alpha);"
  "}";

internal_lnk bool 
gfx_init(Arena *arena, GFX_State *gfx)
{
  Assert(gfx);

  gfx->render_buffer.vertices = arena_push_array(arena, Render_Vertex, MAX_VERTEX_COUNT);
  gfx->render_buffer.indices  = arena_push_array(arena, u32, MAX_VERTEX_COUNT);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_MULTISAMPLE);

  glGenVertexArrays(1, &gfx->vao);
  glBindVertexArray(gfx->vao);

  glGenBuffers(1, &gfx->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo);
  glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_COUNT * VTX_SIZE, NULL, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, false, VTX_SIZE, (void*) OffsetOf(Render_Vertex, position));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, VTX_SIZE, (void*) OffsetOf(Render_Vertex, color));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, false, VTX_SIZE, (void*) OffsetOf(Render_Vertex, uv));
  glEnableVertexAttribArray(2);

  glVertexAttribPointer(3, 2, GL_FLOAT, false, VTX_SIZE, (void*) OffsetOf(Render_Vertex, circ_mask));
  glEnableVertexAttribArray(3);

  glVertexAttribPointer(4, 1, GL_FLOAT, false, VTX_SIZE, (void*) OffsetOf(Render_Vertex, tex_id));
  glEnableVertexAttribArray(4);

  glGenBuffers(1, &gfx->ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_VERTEX_COUNT * sizeof(u32), NULL, GL_DYNAMIC_DRAW);

  gfx->program = glCreateProgram();

  u32 vtx_shader = glCreateShader(GL_VERTEX_SHADER);;
  glShaderSource(vtx_shader, 1, &VERTEX_SHADER, NULL);
  glCompileShader(vtx_shader);

  i32 success;
  char infoLog[512];
  glGetShaderiv(vtx_shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(vtx_shader, 512, NULL, infoLog);
    LogInfo("Vertex Shader compilation Failed\n%s\n", infoLog);
    return false;
  }

  u32 frg_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frg_shader, 1, &FRAGMENT_SHADER, NULL);
  glCompileShader(frg_shader);

  glGetShaderiv(frg_shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(frg_shader, 512, NULL, infoLog);
    LogInfo("Fragment Shader Compilation Failed\n%s\n", infoLog);
    return false;
  }

  glAttachShader(gfx->program, vtx_shader);
  glAttachShader(gfx->program, frg_shader);
  glLinkProgram(gfx->program);

  glGetProgramiv(gfx->program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(gfx->program, 512, NULL, infoLog);
    LogInfo("Shader program Linking failed\n%s\n", infoLog);
    return false;
  }

  glDeleteShader(frg_shader);
  glDeleteShader(vtx_shader);

  glUseProgram(gfx->program);

  gfx->uniform_loc[Uniform_Textures] = glGetUniformLocation(gfx->program, "Textures");
  gfx->uniform_loc[Uniform_ProjMatrix] = glGetUniformLocation(gfx->program, "ProjMatrix");

  return true;
}

internal_lnk void
gfx_resize_target(GFX_State *gfx, i32 w, i32 h)
{
  glViewport(0, 0, w, h);
  f32 proj[16] = {
    2.0f/w,  0.0f,     0.0f,  0.0f,
    0.0f,    2.0f/h,   0.0f,  0.0f,
    0.0f,    0.0f,    -1.0f,  0.0f,
    -1.0f,  -1.0f,     0.0f,  1.0f
  };
  glUniformMatrix4fv(gfx->uniform_loc[Uniform_ProjMatrix], 1, false, proj);
  gfx->viewport.w = w;
  gfx->viewport.h = h;
}

internal_lnk void
gfx_begin_frame(GFX_State *gfx, color8_t color)
{
  glClearColor(color_r(color)/255.0, color_g(color)/255.0, color_b(color)/255.0, color_a(color)/255.0);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  gfx_prep(gfx);

  glUseProgram(gfx->program);
  glBindVertexArray(gfx->vao);

  glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->ibo);
}

internal_lnk void
gfx_end_frame(GFX_State *gfx)
{
  gfx_flush(gfx);
}

internal_lnk void 
gfx_prep(GFX_State *gfx)
{
  gfx->render_buffer.vtx_count = 0;
  gfx->render_buffer.idx_count = 0;
}

internal_lnk void
gfx_flush(GFX_State *gfx)
{
  glBufferSubData(GL_ARRAY_BUFFER, 0, gfx->render_buffer.vtx_count * VTX_SIZE, gfx->render_buffer.vertices);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, gfx->render_buffer.idx_count * sizeof(u32), gfx->render_buffer.indices);
  glDrawElements(GL_TRIANGLES, gfx->render_buffer.idx_count, GL_UNSIGNED_INT, NULL);
}

internal_lnk void
gfx_push_rect(GFX_State *gfx, vec2_f32 pos, vec2_f32 size, color8_t color)
{
  if (gfx->render_buffer.vtx_count + 4 > MAX_VERTEX_COUNT ||
      gfx->render_buffer.idx_count + 6 > MAX_VERTEX_COUNT)
  {
    gfx_flush(gfx);
    gfx_prep(gfx);
  }
  u32 base_idx = gfx->render_buffer.vtx_count;
  Render_Vertex *vtx = gfx->render_buffer.vertices + base_idx;
  u32 *idx = gfx->render_buffer.indices + gfx->render_buffer.idx_count;
  vec2_f32 min = pos;
  vec2_f32 max = {pos.x + size.x, pos.y + size.y};

  color = hex_color(color);
  
  vtx[0].position = min;
  vtx[0].color = color;
  vtx[0].uv = (vec2_f32){0.0f, 0.0f};
  vtx[0].circ_mask = (vec2_f32){0.0f, 0.0f};
  vtx[0].tex_id = 0.0f;
  
  vtx[1].position = (vec2_f32){max.x, min.y};
  vtx[1].color = color;
  vtx[1].uv = (vec2_f32){1.0f, 0.0f};
  vtx[1].circ_mask = (vec2_f32){0.0f, 0.0f};
  vtx[1].tex_id = 0.0f;

  vtx[2].position = max;
  vtx[2].color = color;
  vtx[2].uv = (vec2_f32){1.0f, 1.0f};
  vtx[2].circ_mask = (vec2_f32){0.0f, 0.0f};
  vtx[2].tex_id = 0.0f;
  
  vtx[3].position = (vec2_f32){min.x, max.y};
  vtx[3].color = color;
  vtx[3].uv = (vec2_f32){0.0f, 1.0f};
  vtx[3].circ_mask = (vec2_f32){0.0f, 0.0f};
  vtx[3].tex_id = 0.0f;
  
  idx[0] = base_idx + 0;
  idx[1] = base_idx + 1;
  idx[2] = base_idx + 2;
  idx[3] = base_idx + 2;
  idx[4] = base_idx + 3;
  idx[5] = base_idx + 0;
  
  gfx->render_buffer.vtx_count += 4;
  gfx->render_buffer.idx_count += 6;
}
