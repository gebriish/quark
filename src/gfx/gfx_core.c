#include "gfx_core.h"

#include "../base/base_string.h"
#include "../thirdparty/glad/glad.c"

#include <stddef.h> // for offsetof

#define VTX_SIZE sizeof(Render_Vertex)

global String8 VERTEX_SHADER_SRC = str8_lit (
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
  "}"
);

global String8 FRAGMENT_SHADER_SRC = str8_lit(
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
  "    vec4 base = Frag_Color * texture(Textures[tex_id], Frag_UV);"
  "    Out_Color = vec4(base.rgb, base.a * alpha);"
  "}"
);

internal_lnk bool 
gfx_init(Arena *allocator, Arena *temp_allocator, GFX_State *gfx)
{
  Assert(gfx);
  MemZeroStruct(gfx);

  gfx->allocator = allocator;
  gfx->temp_allocator = temp_allocator;

  gfx->render_buffer.vertices = arena_push_array(allocator, Render_Vertex, MAX_VERTEX_COUNT);
  gfx->render_buffer.indices  = arena_push_array(allocator, u16, MAX_VERTEX_COUNT);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_MULTISAMPLE);

  glGenVertexArrays(1, &gfx->vao);
  glBindVertexArray(gfx->vao);

  glGenBuffers(1, &gfx->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo);
  glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_COUNT * VTX_SIZE, NULL, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, false, VTX_SIZE, (void*) offsetof(Render_Vertex, position));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, VTX_SIZE, (void*) offsetof(Render_Vertex, color));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, true, VTX_SIZE, (void*) offsetof(Render_Vertex, uv));
  glEnableVertexAttribArray(2);

  glVertexAttribPointer(3, 2, GL_BYTE, true, VTX_SIZE, (void*) offsetof(Render_Vertex, circ_mask));
  glEnableVertexAttribArray(3);

  glVertexAttribPointer(4, 1, GL_UNSIGNED_BYTE, false, VTX_SIZE, (void*) offsetof(Render_Vertex, tex_id));
  glEnableVertexAttribArray(4);

  glGenBuffers(1, &gfx->ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_VERTEX_COUNT * sizeof(u16), NULL, GL_DYNAMIC_DRAW);

  gfx->program = glCreateProgram();

  u32 vtx_shader = glCreateShader(GL_VERTEX_SHADER);;
  glShaderSource(vtx_shader, 1, (const GLchar *const*)&VERTEX_SHADER_SRC.raw, NULL);
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
  glShaderSource(frg_shader, 1, (const GLchar *const*)&FRAGMENT_SHADER_SRC.raw, NULL);
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

  i32 samplers[MAX_TEXTURES] = {0};
  for (i32 i=0; i<MAX_TEXTURES; i++) { samplers[i] = i; } 

  glUniform1iv(gfx->uniform_loc[Uniform_Textures], MAX_TEXTURES, samplers);

  u32 white_pixel = 0xffffffff;
  Texture_Data data_white = {
    .data = (u8 *)&white_pixel,
    .width = 1,
    .height = 1,
    .channels = 4,
  };
  gfx_texture_upload(gfx, data_white, TextureKind_Normal);

  return true;
}

internal_lnk void
gfx_resize_target(GFX_State *gfx, i32 w, i32 h)
{
  glViewport(0, 0, w, h);
  f32 proj[16] = {
    2.0f/(f32)w,  0.0f,     0.0f,  0.0f,
    0.0f,   -2.0f/(f32)h,   0.0f,  0.0f,
    0.0f,    0.0f,    -1.0f,  0.0f,
    -1.0f,   1.0f,     0.0f,  1.0f
  };

  glUniformMatrix4fv(gfx->uniform_loc[Uniform_ProjMatrix], 1, false, proj);
  gfx->viewport.w = w;
  gfx->viewport.h = h;
}

internal_lnk void
gfx_begin_frame(GFX_State *gfx, color8_t color)
{
  glClearColor(
    (f32)color_r(color)/255.0f, 
    (f32)color_g(color)/255.0f, 
    (f32)color_b(color)/255.0f,
    (f32)color_a(color)/255.0f
  );
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
  glDrawElements(GL_TRIANGLES, (int)(gfx->render_buffer.idx_count), GL_UNSIGNED_SHORT, NULL);
}

internal_lnk void
gfx_push_rect(GFX_State *gfx, Rect_Params *params)
{
  if (gfx->render_buffer.vtx_count + 4 > MAX_VERTEX_COUNT ||
    gfx->render_buffer.idx_count + 6 > MAX_VERTEX_COUNT)
  {
    gfx_flush(gfx);
    gfx_prep(gfx);
  }

  u16 base_idx = (u16)(gfx->render_buffer.vtx_count);

  Render_Vertex *vtx = gfx->render_buffer.vertices + base_idx;
  u16 *idx = gfx->render_buffer.indices + gfx->render_buffer.idx_count;

  vec2_f32 pos = params->position;
  color8_t color = params->color;
  vec2_f32 size = params->size;
  vec4_u16 uv = {
    (u16)(params->uv.x * U16_MAX),
    (u16)(params->uv.y * U16_MAX),
    (u16)(params->uv.z * U16_MAX),
    (u16)(params->uv.w * U16_MAX),
  };

  f32 x0 = pos.x, y0 = pos.y;
  f32 x1 = pos.x + size.x, y1 = pos.y + size.y;
  color8_t final_color = hex_color(color);

  vtx[0] = (Render_Vertex){
    .position = {x0, y0},
    .color = final_color,
    .uv = {uv.x, uv.y},
    .circ_mask = {0, 0},
    .tex_id = params->tex_id
  };

  vtx[1] = (Render_Vertex){
    .position = {x1, y0},
    .color = final_color,
    .uv = {uv.z, uv.y},
    .circ_mask = {0, 0},
    .tex_id = params->tex_id
  };

  vtx[2] = (Render_Vertex){
    .position = {x1, y1},
    .color = final_color,
    .uv = {uv.z, uv.w},
    .circ_mask = {0, 0},
    .tex_id = params->tex_id
  };

  vtx[3] = (Render_Vertex){
    .position = {x0, y1},
    .color = final_color,
    .uv = {uv.x, uv.w},
    .circ_mask = {0, 0},
    .tex_id = params->tex_id
  };

  idx[0] = base_idx;
  idx[1] = base_idx + 1;
  idx[2] = base_idx + 2;
  idx[3] = base_idx + 2;
  idx[4] = base_idx + 3;
  idx[5] = base_idx;

  gfx->render_buffer.vtx_count += 4;
  gfx->render_buffer.idx_count += 6;
}

internal_lnk void
gfx_push_rect_rounded(GFX_State *gfx, Rect_Params *params)
{
  if (params->size.x <= 0.0f || params->size.y <= 0.0f) return;

  vec2_f32 size = params->size;
  vec2_f32 pos = params->position;
  u8 tex_id = params->tex_id;
  color8_t color = hex_color(params->color);

  f32 inv_width = 1.0f / size.x;
  f32 inv_height = 1.0f / size.y;
  f32 uv_base_x = params->uv.x;
  f32 uv_base_y = params->uv.y;
  f32 uv_scale_x = (params->uv.z - params->uv.x) * inv_width;
  f32 uv_scale_y = (params->uv.w - params->uv.y) * inv_height;

  f32 r[4] = {
    params->radii.top_left,
    params->radii.top_right,
    params->radii.bottom_right,
    params->radii.bottom_left
  };

  // Adjust radii to prevent overlapping corners
  // Top side (top-left + top-right)
  f32 top_sum = r[0] + r[1];
  if (top_sum > size.x) {
    f32 scale = size.x / top_sum;
    r[0] *= scale;
    r[1] *= scale;
  }

  // Bottom side (bottom-left + bottom-right)
  f32 bottom_sum = r[3] + r[2];
  if (bottom_sum > size.x) {
    f32 scale = size.x / bottom_sum;
    r[3] *= scale;
    r[2] *= scale;
  }

  // Left side (top-left + bottom-left)
  f32 left_sum = r[0] + r[3];
  if (left_sum > size.y) {
    f32 scale = size.y / left_sum;
    r[0] *= scale;
    r[3] *= scale;
  }

  // Right side (top-right + bottom-right)
  f32 right_sum = r[1] + r[2];
  if (right_sum > size.y) {
    f32 scale = size.y / right_sum;
    r[1] *= scale;
    r[2] *= scale;
  }

  u8 rounded_mask = 0;
  u8 rounded_count = 0;

  rounded_mask |= (r[0] > 0.5f) << 0;
  rounded_mask |= (r[1] > 0.5f) << 1;
  rounded_mask |= (r[2] > 0.5f) << 2;
  rounded_mask |= (r[3] > 0.5f) << 3;

  rounded_count = (rounded_mask & 1) + ((rounded_mask >> 1) & 1) + 
    ((rounded_mask >> 2) & 1) + ((rounded_mask >> 3) & 1);

  u32 outline_vertices = 4 + rounded_count;
  u32 corner_vertices = rounded_count * 3;
  u32 total_vertices = outline_vertices + corner_vertices;
  u32 total_indices = (outline_vertices - 2) * 3 + rounded_count * 3;

  if (gfx->render_buffer.vtx_count + total_vertices > MAX_VERTEX_COUNT ||
    gfx->render_buffer.idx_count + total_indices > MAX_VERTEX_COUNT) {
    gfx_flush(gfx);
    gfx_prep(gfx);
  }

  u16 base_idx = (u16)gfx->render_buffer.vtx_count;
  Render_Vertex *vtx = &gfx->render_buffer.vertices[base_idx];
  u16 *idx = &gfx->render_buffer.indices[gfx->render_buffer.idx_count];

  static const vec2_f32 cw[4] = {
    { 1.0f,  0.0f}, { 0.0f,  1.0f}, {-1.0f,  0.0f}, { 0.0f, -1.0f}
  };
  static const vec2_f32 ccw[4] = {
    { 0.0f,  1.0f}, {-1.0f,  0.0f}, { 0.0f, -1.0f}, { 1.0f,  0.0f}
  };

  f32 right = pos.x + size.x;
  f32 bottom = pos.y + size.y;
  vec2_f32 corners[4] = {
    {pos.x, pos.y},      // top-left
    {right, pos.y},      // top-right
    {right, bottom},     // bottom-right
    {pos.x, bottom}      // bottom-left
  };

#define WRITE_VERTEX(v_ptr, px, py, cmask) do { \
  f32 lx = (px) - pos.x; \
  f32 ly = (py) - pos.y; \
  (v_ptr)->position.x = (px); \
  (v_ptr)->position.y = (py); \
  (v_ptr)->uv.x = (u16)((uv_base_x + lx * uv_scale_x) * U16_MAX); \
  (v_ptr)->uv.y = (u16)((uv_base_y + ly * uv_scale_y) * U16_MAX); \
  (v_ptr)->color = color; \
  (v_ptr)->circ_mask = (cmask); \
  (v_ptr)->tex_id = tex_id; \
} while(0)

  u16 v_idx = 0;

  for (i32 i = 0; i < 4; i++) {
    vec2_f32 corner = corners[i];
    f32 radius = r[i];

    if (!(rounded_mask & (1 << i))) {
      WRITE_VERTEX(&vtx[v_idx], corner.x, corner.y, ((vec2_i8){0, 0}));
      v_idx++;
    } else {
      f32 dx_ccw = ccw[i].x * radius;
      f32 dy_ccw = ccw[i].y * radius;
      f32 dx_cw = cw[i].x * radius;
      f32 dy_cw = cw[i].y * radius;

      WRITE_VERTEX(&vtx[v_idx], corner.x + dx_ccw, corner.y + dy_ccw, ((vec2_i8){0, 0}));
      v_idx++;
      WRITE_VERTEX(&vtx[v_idx], corner.x + dx_cw, corner.y + dy_cw, ((vec2_i8){0, 0}));
      v_idx++;
    }
  }

  u32 idx_count = 0;
  u16 fan_center = base_idx;
  for (u16 i = 1; i < outline_vertices - 1; i++) {
    idx[idx_count++] = fan_center;
    idx[idx_count++] = base_idx + i + 1;
    idx[idx_count++] = base_idx + i;
  }

  for (i32 i = 0; i < 4; i++) {
    if (!(rounded_mask & (1 << i))) continue;

    vec2_f32 corner = corners[i];
    f32 radius = r[i];

    f32 dx_ccw = ccw[i].x * radius;
    f32 dy_ccw = ccw[i].y * radius;
    f32 dx_cw = cw[i].x * radius;
    f32 dy_cw = cw[i].y * radius;

    WRITE_VERTEX(&vtx[v_idx], corner.x, corner.y, ((vec2_i8){I8_MAX, I8_MAX}));
    u16 center_v = v_idx++;

    WRITE_VERTEX(&vtx[v_idx], corner.x + dx_ccw, corner.y + dy_ccw, ((vec2_i8){0, I8_MAX}));
    u16 edge1_v = v_idx++;

    WRITE_VERTEX(&vtx[v_idx], corner.x + dx_cw, corner.y + dy_cw, ((vec2_i8){I8_MAX, 0}));
    u16 edge2_v = v_idx++;

    idx[idx_count++] = base_idx + center_v;
    idx[idx_count++] = base_idx + edge1_v;
    idx[idx_count++] = base_idx + edge2_v;
  }

  #undef WRITE_VERTEX

  gfx->render_buffer.vtx_count += total_vertices;
  gfx->render_buffer.idx_count += total_indices;
}


internal_lnk u32 
gfx_texture_upload(GFX_State *gfx, Texture_Data data, Texture_Kind type)
{
  Assert(gfx && type >= 0 && type < TextureKind_Count);
  Assert(data.data && data.channels <= 4 && data.channels >= 1 && data.width > 0 && data.height > 0);

  u32 result = 0;
  Texture *handle = NULL;

  if (gfx->texture_freelist != 0) {
    result = gfx->texture_freelist;
    handle = &gfx->texture_slots[result];
    gfx->texture_freelist = handle->next;
  } else {
    result = gfx->texture_count;
    if (result >= MAX_TEXTURES) { 
      return WHITE_TEXTURE;
    }
    gfx->texture_count += 1;
  }

  handle = &gfx->texture_slots[result];
  *handle = (Texture){0};

  glGenTextures(1, &handle->gl_id);

  i32 internal_format;
  u32 format;
  switch (data.channels) {
    case 1:
      internal_format = GL_RED;
      format = GL_RED;
      break;  
    case 2:
      internal_format = GL_RG;
      format = GL_RG;
      break;  
    case 3:
      internal_format = GL_RGB;
      format = GL_RGB;
      break;  
    case 4:
      internal_format = GL_RGBA;
      format = GL_RGBA;
      break;  
    default:
      glDeleteTextures(1, &handle->gl_id);
      handle->gl_id = 0;
      handle->next = gfx->texture_freelist;
      gfx->texture_freelist = result;
      return WHITE_TEXTURE;
  }

  glActiveTexture(GL_TEXTURE0 + result);
  glBindTexture(GL_TEXTURE_2D, handle->gl_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (type == TextureKind_GreyScale && data.channels == 1) {
    i32 swizzle[4] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
  }

  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    internal_format,
    data.width,
    data.height,
    0,
    format,
    GL_UNSIGNED_BYTE,
    data.data
  );

  glGenerateMipmap(GL_TEXTURE_2D);

  return result;
}


internal_lnk bool 
gfx_texture_update(GFX_State *gfx, u32 id, i32 w, i32 h, i32 channels, u8 *data)
{
  Assert(gfx && data && w > 0 && h > 0 && channels > 0);

  if (id >= gfx->texture_count) {
    return false;
  }
  
  Texture *handle = &gfx->texture_slots[id];
  if (handle->gl_id == 0) {
    return false;
  }
  
  u32 format;
  switch (channels) {
    case 1: format = GL_RED; break;
    case 2: format = GL_RG; break;
    case 3: format = GL_RGB; break;
    case 4: format = GL_RGBA; break;
    default: return false;
  }
  
  glActiveTexture(GL_TEXTURE0 + id);
  glBindTexture(GL_TEXTURE_2D, handle->gl_id);
  glTexSubImage2D(
    GL_TEXTURE_2D,
    0,
    0, 0,
    w,
    h,
    format,
    GL_UNSIGNED_BYTE,
    data
  );
  glGenerateMipmap(GL_TEXTURE_2D);
  return true;
}

