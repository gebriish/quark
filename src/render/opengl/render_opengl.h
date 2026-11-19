#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

//////////////////////////////////////
// @section: opengl proc list

#define R_OGL_ProcedureXList \
  X(glGenBuffers, void, (GLsizei n, GLuint *buffers))\
  X(glBindBuffer, void, (GLenum target, GLuint buffer))\
  X(glDeleteBuffers, void, (GLsizei n, GLuint *buffers))\
  X(glGenVertexArrays, void, (GLsizei n, GLuint *arrays))\
  X(glBindVertexArray, void, (GLuint array))\
  X(glGenFramebuffers, void, (GLsizei n, GLuint *fbos))\
  X(glDeleteFramebuffers, void, (GLsizei n, GLuint *fbos))\
  X(glCreateProgram, GLuint, (void))\
  X(glCreateShader, GLuint, (GLenum type))\
  X(glShaderSource, void, (GLuint shader, GLsizei count, char **string, GLint *length))\
  X(glCompileShader, void, (GLuint shader))\
  X(glGetShaderiv, void, (GLuint shader, GLenum pname, GLint *params))\
  X(glGetShaderInfoLog, void, (GLuint shader, GLsizei bufSize, GLsizei *length, char *infoLog))\
  X(glGetProgramiv, void, (GLuint program, GLenum pname, GLint *params))\
  X(glGetProgramInfoLog, void, (GLuint program, GLsizei bufSize, GLsizei *length, char *infoLog))\
  X(glAttachShader, void, (GLuint program, GLuint shader))\
  X(glLinkProgram, void, (GLuint program))\
  X(glValidateProgram, void, (GLuint program))\
  X(glDeleteShader, void, (GLuint shader))\
  X(glUseProgram, void, (GLuint program))\
  X(glGetUniformLocation, GLint, (GLuint program, char *name))\
  X(glGetAttribLocation, GLint, (GLuint program, char *name))\
  X(glEnableVertexAttribArray, void, (GLuint index))\
  X(glVertexAttribPointer, void, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer))\
  X(glBufferData, void, (GLenum target, ptrdiff_t size, void *data, GLenum usage))\
  X(glBufferSubData, void, (GLenum target, ptrdiff_t offset, ptrdiff_t size, const void *data))\
  X(glBlendFuncSeparate, void, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha))\
  X(glUniform1f, void, (GLint location, GLfloat v0))\
  X(glUniform2f, void, (GLint location, GLfloat v0, GLfloat v1))\
  X(glUniform3f, void, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2))\
  X(glUniform4f, void, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))\
  X(glUniform4fv, void, (GLint location, GLsizei count, const GLfloat *value))\
  X(glUniformMatrix3fv, void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))\
  X(glUniformMatrix4fv, void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))\
  X(glUniform1i, void, (GLint location, GLint v0))\
  X(glGenerateMipmap, void, (GLenum target))\
  X(glTexSubImage2D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels))\
  X(glBindAttribLocation, void, (GLuint programObj, GLuint index, char *name))\
  X(glBindFragDataLocation, void, (GLuint program, GLuint color, char *name))\
  X(glBindFramebuffer, void, (GLenum target, GLuint fbo))\
  X(glActiveTexture, void, (GLenum texture))\
  X(glVertexAttribDivisor, void, (GLuint index, GLuint divisor))\
  X(glDrawArraysInstanced, void, (GLenum mode, GLint first, GLsizei count, GLsizei instancecount))\
  X(glDebugMessageCallback, void, (void (*)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam), void *user_data))\
  X(glFramebufferTexture2D, void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level))\
  X(glBlitFramebuffer, void, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter))\

#define X(name, r, p) typedef r name##_FunctionType p;
R_OGL_ProcedureXList
#undef X

#define X(name, r, p) global name##_FunctionType *name = 0;
R_OGL_ProcedureXList
#undef X

//////////////////////////////////////
// @section: types

typedef u32 R_OGL_ShaderKind;
enum
{
  R_OGL_ShaderKind_Quad,
  R_OGL_ShaderKind_Blur,
  R_OGL_ShaderKind_Text,
  R_OGL_ShaderKind_COUNT,
};

typedef struct R_OGL_Buffer R_OGL_Buffer;
struct R_OGL_Buffer
{
  GLuint id;
  R_OGL_Buffer *next;
};

typedef struct R_OGL_Target R_OGL_Target;
struct R_OGL_Target
{
  GLuint fbo;
  GLuint tex;
  s32 w;
  s32 h;
};

typedef struct R_OGL_Texture R_OGL_Texture;
struct R_OGL_Texture
{
  GLuint id;
  R_TexFormat fmt;
  Vec2S32 dim;
  R_OGL_Texture *next;
};

typedef struct R_OGL_Attr R_OGL_Attr;
struct R_OGL_Attr 
{ 
  GLint n; 
  size_t off; 
};

typedef struct R_OGL_State R_OGL_State;
struct R_OGL_State
{
  Arena *arena;
  Arena *frame_arena;
  GLuint gen_purpose_vao;
  GLuint scratch_vbo;
  GLuint shaders[R_OGL_ShaderKind_COUNT];
  R_OGL_Buffer *flush_buffers;
  R_OGL_Target targets[2];
  R_OGL_Texture *texture_free;
  R_Handle white_tex;
};

typedef struct R_OGL_ClipRect R_OGL_ClipRect;
struct R_OGL_ClipRect
{
  Rng2S32 rect;
  R_OGL_ClipRect *next;
};

global R_OGL_State *r_ogl_state = 0;

//////////////////////////////////////
// @section: ogl helper functions

function GLuint r_ogl_compile_shader(String8 vert, String8 frag);
function GLuint r_ogl_instance_buffer_from_size(u64 size);
function void   r_ogl_target_generate(R_OGL_Target *t, s32 w, s32 h);
function void   r_ogl_draw_instances(GLuint shader, void *data, s64 count, s64 stride, R_OGL_Attr *attrs, s64 attr_count, R_Handle tex);

//////////////////////////////////////
// @section: target defined functions

function void r_ogl_os_init(void);
function void r_ogl_select_window(GFX_Window window);
function void r_ogl_window_swap_buffers(GFX_Window window);
function void *r_ogl_os_load_procedure(const char *name);
function void r_ogl_set_scissor(Rng2S32 rect);

//////////////////////////////////////
// @section: macros

#define glBindVertexArrayScope(id) DeferLoop(glBindVertexArray(id), glBindVertexArray(0))
#define glBindBufferScope(buf, id) DeferLoop(glBindBuffer(buf, id), glBindBuffer(buf, 0))
#define glUseProgramScope(prog) DeferLoop(glUseProgram(prog), glUseProgram(0))
#define glBindTextureScope(type, id) DeferLoop(glBindTexture(type, id), glBindTexture(type, 0))
#define glBindFramebufferScope(type, id) DeferLoop(glBindFramebuffer(type, id), glBindFramebuffer(type, 0))

//////////////////////////////////////
// @section: glsl shaders

global read_only String8 r_ogl_quad_vertex_glsl = S(
"#version 330 core\n"

"layout(location = 0) in vec4  dst;"
"layout(location = 1) in vec4  src;"
"layout(location = 2) in vec4  col;"
"layout(location = 3) in vec4  radii;"
"layout(location = 4) in float thickness;"
"layout(location = 5) in float edge_blur;"

"out VS_OUT {"
  "vec2  uv;"
  "vec4  color;"
  "vec4  radii;"
  "vec2  local;"
  "vec2  half_size;"
  "float thickness;"
  "float edge_blur;"
"} vs_out;"

"uniform mat4 projection;"

"const vec2 corners[4] = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));"

"void main()"
"{"
  "vec2 pos  = corners[gl_VertexID];"
  "vec2 uv   = src.xy + (src.zw - src.xy) * pos;"
  "vec2 size = dst.zw - dst.xy;"
  "gl_Position = projection * vec4(dst.xy + pos * size, 0.0, 1.0);"
  "vs_out.color     = col;"
  "vs_out.radii     = radii;"
  "vs_out.local     = (pos - 0.5) * size;"
  "vs_out.half_size = size * 0.5;"
  "vs_out.thickness = thickness;"
  "vs_out.edge_blur = edge_blur;"
  "vs_out.uv        = uv;"
"}"
);

global read_only String8 r_ogl_quad_fragment_glsl = S(
"#version 330 core\n"

"in VS_OUT {"
  "vec2  uv;"
  "vec4  color;"
  "vec4  radii;"
  "vec2  local;"
  "vec2  half_size;"
  "float thickness;"
  "float edge_blur;"
"} fs_in;"

"out vec4 frag_color;"

"uniform sampler2D tex;"

"float coverage(float d, float aa)"
"{"
  "return 1.0 - smoothstep(-aa, aa, d);"
"}"

"void main()"
"{"
  "vec2 p = fs_in.local;"
  "vec2 b = fs_in.half_size;"
  "vec2 corners = fs_in.local.x >= 0.0 ? fs_in.radii.zy : fs_in.radii.wx;"
  "float r = min(fs_in.local.y >= 0.0 ? corners.x : corners.y, min(b.x, b.y));"
  
  "vec2 q = abs(p) - b + r;"
  "float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;"
  
  "vec4 pixel_col = texture(tex, fs_in.uv);"
  "float blur_value = max(fs_in.edge_blur, 0.5);"
  "float outline = fs_in.thickness;"
  "float a_outer = 1.0 - smoothstep(-blur_value, 0.5, d);"
  "float a_inner = (1.0 - smoothstep(-blur_value, 0.5, d + outline)) * step(0.5, outline);"
  "float alpha = a_outer - a_inner;"
  
  "frag_color = vec4(fs_in.color.rgb, fs_in.color.a * alpha) * pixel_col;"
"}"
);


global read_only String8 r_ogl_blur_vertex_glsl = S(
"#version 330 core\n"

"out VS_OUT {"
  "vec2 uv;"
  "vec2 local;"
  "vec2 half_size;"
"} vs_out;"

"uniform vec4 rect;"
"uniform vec2 viewport_size;"
"uniform vec2 direction;"
"uniform int  blur_count;"

"const vec2 corners[4] = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));"

"void main()"
"{"
  "vec2 pos   = corners[gl_VertexID];"
  "vec2 size  = rect.zw - rect.xy;"
  "vec2 pad   = direction.x > 0.0 ? vec2(0.0, float(blur_count) + 1.0) : vec2(0.0);"
  "vec2 psize = size + 2.0 * pad;"
  "vec2 p     = rect.xy - pad + pos * psize;"
  "gl_Position      = vec4(2.0 * p.x / viewport_size.x - 1.0, 1.0 - 2.0 * p.y / viewport_size.y, 0.0, 1.0);"
  "vs_out.uv        = vec2(p.x, viewport_size.y - p.y) / viewport_size;"
  "vs_out.local     = (pos - 0.5) * psize;"
  "vs_out.half_size = size * 0.5;"
"}"
);

global read_only String8 r_ogl_blur_fragment_glsl = S(
"#version 330 core\n"

"in VS_OUT {"
  "vec2 uv;"
  "vec2 local;"
  "vec2 half_size;"
"} fs_in;"

"out vec4 frag_color;"

"uniform sampler2D tex;"
"uniform vec2      direction;"
"uniform int       blur_count;"
"uniform vec4      kernel[32];"
"uniform vec4      tint;"
"uniform vec4      corner_radii_px;"

"void main()"
"{"
  "vec3 rgb = texture(tex, fs_in.uv).rgb * kernel[0].x;"
  "for (int i = 1; i < blur_count; i += 2)"
  "{"
    "float w = kernel[(i + 1) / 2].x;"
    "float o = kernel[(i + 1) / 2].y;"
    "rgb += w * texture(tex, fs_in.uv - o * direction).rgb;"
    "rgb += w * texture(tex, fs_in.uv + o * direction).rgb;"
  "}"

  "if (direction.x > 0.0)"
  "{"
    "frag_color = vec4(rgb, 1.0);"
    "return;"
  "}"
  
  "vec2  corners = fs_in.local.x >= 0.0 ? corner_radii_px.zy : corner_radii_px.wx;"
  "float radius  = min(fs_in.local.y >= 0.0 ? corners.x : corners.y, min(fs_in.half_size.x, fs_in.half_size.y));"
  "vec2  q       = abs(fs_in.local) - fs_in.half_size + radius;"
  "float d       = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;"
  "float alpha   = 1.0 - smoothstep(-0.5, 0.5, d);"
  "frag_color = vec4(rgb, alpha) * tint;"
"}"
);

global read_only String8 r_ogl_text_vertex_glsl = S(
"#version 330 core\n"
"layout(location = 0) in vec4 dst;"
"layout(location = 1) in vec4 src;"
"layout(location = 2) in vec4 fg;"

"out VS_OUT { vec2 uv; vec4 fg; } vs_out;"

"uniform mat4 projection;"
"const vec2 corners[4] = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));"

"void main()"
"{"
  "vec2 pos  = corners[gl_VertexID];"
  "vec2 size = dst.zw - dst.xy;"
  "gl_Position = projection * vec4(dst.xy + pos * size, 0.0, 1.0);"
  "vs_out.uv = src.xy + (src.zw - src.xy) * pos;"
  "vs_out.fg = fg;"
"}"
);

global read_only String8 r_ogl_text_fragment_glsl = S(
"#version 330 core\n"
"in VS_OUT { vec2 uv; vec4 fg; } fs_in;"

"layout(location = 0, index = 0) out vec4 frag_color;"
"layout(location = 0, index = 1) out vec4 frag_blend;"

"uniform sampler2D tex;"

"void main()"
"{"
  "vec3 cov = texture(tex, fs_in.uv).rgb * fs_in.fg.a;"
  "float a  = max(cov.r, max(cov.g, cov.b));"
  "if (a <= 0.0) discard;"
  "frag_color = vec4(fs_in.fg.rgb * cov, a);"
  "frag_blend = vec4(cov, a);"
"}"
);

#endif // RENDER_OPENGL_H
