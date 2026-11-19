
function void
r_init(void)
{
  // setting up the opengl context
  r_ogl_os_init();

  // loading opengl procedures from os
#define X(name, r, p) name = (name##_FunctionType *)r_ogl_os_load_procedure(#name);
  R_OGL_ProcedureXList
#undef X

  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,   // color
                      GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);  // alpha

  // internal opengl renderer state setup.
  Arena *arena = arena_reserve(Megabytes(4));
  Arena *frame = arena_reserve(Megabytes(4));

  r_ogl_state = arena_push(arena, R_OGL_State, 1);
  r_ogl_state->arena = arena;
  r_ogl_state->frame_arena = frame;

  // setup quad batch buffer

  {
    glGenVertexArrays(1, &r_ogl_state->gen_purpose_vao);

    glGenBuffers(1, &r_ogl_state->scratch_vbo);
    glBindBufferScope(GL_ARRAY_BUFFER, r_ogl_state->scratch_vbo)
    {
      glBufferData(GL_ARRAY_BUFFER, QUAD_BATCH_SIZE * sizeof(R_Quad), 0, GL_DYNAMIC_DRAW);
    }
  }

  // compile all shaders
  {
    struct { String8 vert; String8 frag; } 
    shaders_src[] = 
    {
      {r_ogl_quad_vertex_glsl,       r_ogl_quad_fragment_glsl},
      {r_ogl_blur_vertex_glsl,       r_ogl_blur_fragment_glsl},
      {r_ogl_text_vertex_glsl,       r_ogl_text_fragment_glsl},
    };

    for EachIndex(i, CArrayLength(shaders_src))
    {
      r_ogl_state->shaders[i] = r_ogl_compile_shader(shaders_src[i].vert, shaders_src[i].frag);
    }
  }

  // texture initialization
  {
    u32 white = 0xffffffff;
    R_Image white_img = {
      .data = (u8 *) &white,
      .dim = { 1, 1 },
      .fmt = R_TexFormat_RGBA8,
    };
    r_ogl_state->white_tex = r_tex_alloc(white_img);
  }

  r_fnt_init();
}

function void 
r_window_begin_frame(GFX_Window win)
{
  r_ogl_select_window(win);

  Vec2F32 size = gfx_window_size(win);

  r_ogl_target_generate(&r_ogl_state->targets[0], (s32)size.x, (s32)size.y);
  r_ogl_target_generate(&r_ogl_state->targets[1], (s32)size.x, (s32)size.y);

  glBindFramebuffer(GL_FRAMEBUFFER, r_ogl_state->targets[0].fbo);
  glViewport(0, 0, (s32)size.x, (s32)size.y);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  Rng3F32 view_box = {
    .min = v3f32(0,size.y,-1),
    .max = v3f32(size.x,0, 1),
  };
  Mat4x4F32 proj = mat4_ortho(view_box);

  for EachIndex(i, R_OGL_ShaderKind_COUNT)
  {
    glUseProgramScope(r_ogl_state->shaders[i])
    {
      GLint proj_loc = glGetUniformLocation(r_ogl_state->shaders[i], "projection");
      glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.raw);
    }
  }
}

function void 
r_window_end_frame(GFX_Window win)
{
  Vec2F32 size = gfx_window_size(win);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, r_ogl_state->targets[0].fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, (s32)size.x, (s32)size.y, 0, 0, (s32)size.x, (s32)size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  r_ogl_window_swap_buffers(win);
}

function void 
r_window_submit_frame(R_DrawList list)
{
  Temp scratch = scratch_begin(0, 0);
  glEnable(GL_SCISSOR_TEST);

  R_OGL_ClipRect *clip_stack = arena_push(scratch.arena, R_OGL_ClipRect, 1);
  clip_stack->rect.max.x = r_ogl_state->targets[0].w;
  clip_stack->rect.max.y = r_ogl_state->targets[0].h;
  r_ogl_set_scissor(clip_stack->rect);

  for EachNode(draw, R_Draw, list.first) switch (draw->kind)
  {
    case R_DrawKind_ClipPush:
    {
      R_OGL_ClipRect *node = arena_push(scratch.arena, R_OGL_ClipRect, 1);
      node->rect = draw->rect;
      SLLStackPush(clip_stack, node);
      r_ogl_set_scissor(node->rect);
    } break;

    case R_DrawKind_ClipPop:
    {
      SLLStackPop(clip_stack);
      r_ogl_set_scissor(clip_stack->rect);
    } break;

    case R_DrawKind_Quads:
    {
      R_QuadBatch *b = &draw->quad_batch;
      if(b->len == 0) break;
      R_OGL_Attr attrs[] = 
      {
        {4, offsetof(R_Quad, dst)      }, {4, offsetof(R_Quad, src)      },
        {4, offsetof(R_Quad, col)      }, {4, offsetof(R_Quad, radii)    },
        {1, offsetof(R_Quad, thickness)}, {1, offsetof(R_Quad, edge_blur)},
      };
      r_ogl_draw_instances(r_ogl_state->shaders[R_OGL_ShaderKind_Quad], b->raw, b->len, sizeof(R_Quad), attrs, CArrayLength(attrs), b->tex);
    } break;

    case R_DrawKind_Text:
    {
      R_TextBatch *b = &draw->text_batch;
      if(b->len == 0) break;

      GLuint shader = r_ogl_state->shaders[R_OGL_ShaderKind_Text];
      glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC1_COLOR, 
                          GL_ONE, GL_ONE_MINUS_SRC1_ALPHA);

      R_OGL_Attr attrs[] =
      {
        {4, offsetof(R_TextQuad, dst)},
        {4, offsetof(R_TextQuad, src)},
        {4, offsetof(R_TextQuad, fg)},
      };
      r_ogl_draw_instances(shader, b->raw, b->len, sizeof(R_TextQuad), attrs, CArrayLength(attrs), b->tex);

      glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, 
                          GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);
    } break;

    case R_DrawKind_Blur:
    {
      R_Blur *params = &draw->blur;
      Vec2F32 viewport_dim = v2f32(r_ogl_state->targets[0].w, r_ogl_state->targets[0].h);
      GLuint shader = r_ogl_state->shaders[R_OGL_ShaderKind_Blur];

      glUseProgramScope(shader) glBindVertexArrayScope(r_ogl_state->gen_purpose_vao)
      {
        // making the blur kernel
        Vec4F32 kernel[64] = {0};
        u64 blur_count = 0;
        {
          f32 weights[CArrayLength(kernel) * 2] = {0};
          f32 blur_size = Min(params->blur, CArrayLength(weights));
          blur_count = (u64) floorf(blur_size);
          f32 stdev = (blur_size-1.f)/2.f;
          f32 one_over_root_2pi_stdev2 = 1/sqrtf(2*3.14159265f*stdev*stdev);
          f32 euler32 = 2.718281828459045f;
          weights[0] = 1.f;
          if(stdev > 0.f)
          {
            for(u64 idx = 0; idx < blur_count; idx += 1)
            {
              f32 kx = (f32)idx;
              weights[idx] = one_over_root_2pi_stdev2*powf(euler32, -kx*kx/(2.f*stdev*stdev));
            }
          }
          if(weights[0] > 1.f)
          {
            MemZero(weights, sizeof(weights));
            weights[0] = 1.f;
          }
          else
          {
            for(u64 idx = 1; idx < blur_count; idx += 2)
            {
              f32 w0 = weights[idx + 0];
              f32 w1 = weights[idx + 1];
              f32 w = w0 + w1;
              f32 t = w1 / w;
              kernel[(idx+1)/2] = v4f32(w, (f32)idx + t, 0, 0);
            }
          }
          kernel[0].x = weights[0];
        }

        // uniforms
        glUniform4fv(glGetUniformLocation(shader, "rect"), 1, (f32 *)&params->rect);
        glUniform4fv(glGetUniformLocation(shader, "corner_radii_px"), 1, (f32 *)&params->radii);
        glUniform4fv(glGetUniformLocation(shader, "tint"), 1, (f32 *)&params->tint);
        glUniform2f(glGetUniformLocation(shader, "viewport_size"), viewport_dim.x, viewport_dim.y);
        glUniform1i(glGetUniformLocation(shader, "blur_count"), (GLint)blur_count);
        glUniform4fv(glGetUniformLocation(shader, "kernel"), CArrayLength(kernel), (f32 *)&kernel[0]);
        glUniform1i(glGetUniformLocation(shader, "tex"), 0);
        glActiveTexture(GL_TEXTURE0);

        // first pass
        glBindFramebuffer(GL_FRAMEBUFFER, r_ogl_state->targets[1].fbo);
        glBindTextureScope(GL_TEXTURE_2D, r_ogl_state->targets[0].tex)
        {
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glUniform2f(glGetUniformLocation(shader, "direction"), 1.f/viewport_dim.x, 0);
          glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // second pass
        glBindFramebuffer(GL_FRAMEBUFFER, r_ogl_state->targets[0].fbo);
        glBindTextureScope(GL_TEXTURE_2D, r_ogl_state->targets[1].tex)
        {
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glUniform2f(glGetUniformLocation(shader, "direction"), 0, 1.f/viewport_dim.y);
          glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
      }
    } break;
  }

  for EachNode(buf, R_OGL_Buffer, r_ogl_state->flush_buffers)
  {
    glDeleteBuffers(1, &buf->id);
  }

  glDisable(GL_SCISSOR_TEST);
  scratch_end(scratch);
}

function Arena *
r_frame_arena(void)
{
  return r_ogl_state->frame_arena;
}

function void 
r_frame_end(void)
{
  r_fnt_frame();
  arena_free(r_ogl_state->frame_arena, false);
}

function R_Handle 
r_tex_alloc(R_Image image)
{
  R_OGL_Texture *tex = r_ogl_state->texture_free;
  if (r_ogl_state->texture_free)
  {
    SLLStackPop(r_ogl_state->texture_free);
  }
  else
  {
    tex = arena_push_non_zero(r_ogl_state->arena, R_OGL_Texture, 1);
  }

  MemZeroStruct(tex);
  tex->fmt = image.fmt;
  tex->dim = image.dim;

  glGenTextures(1, &tex->id);
  glBindTextureScope(GL_TEXTURE_2D, tex->id)
  {
    struct { GLint internal; GLenum format; }
    fmt_map[] = 
    {
      [R_TexFormat_R8]    = { GL_R8,    GL_RED  },
      [R_TexFormat_RG8]   = { GL_RG8,   GL_RG   },
      [R_TexFormat_RGB8]  = { GL_RGB8,  GL_RGB  },
      [R_TexFormat_RGBA8] = { GL_RGBA8, GL_RGBA },
    };

    GLint  internal = fmt_map[image.fmt].internal;
    GLenum format   = fmt_map[image.fmt].format;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internal, image.dim.x, image.dim.y, 0, format, GL_UNSIGNED_BYTE, (void *) image.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  R_Handle result = {0};
  result.v[0] = (u64) tex;
  return result;
}

function Vec2S32
r_tex_size(R_Handle h)
{
	R_OGL_Texture *tex = (R_OGL_Texture *) h.v[0];
	return tex->dim;
}

function void
r_tex_free(R_Handle h)
{
	R_OGL_Texture *t = (R_OGL_Texture *)h.v[0];
	if (t != 0 && t->id != 0)
	{
		glDeleteTextures(1, &t->id);
		SLLStackPush(r_ogl_state->texture_free, t);
	}
}

function void
r_tex_update(R_Handle h, Rng2S32 range, u8 *data)
{
	R_OGL_Texture *tex = (R_OGL_Texture *) h.v[0];

	s32 x = range.begin[0];
	s32 y = range.begin[1];

	s32 width  = range.end[0] - x;
	s32 height = range.end[1] - y;

	glBindTextureScope(GL_TEXTURE_2D, tex->id)
	{
		GLenum fmt_map[] = 
		{
			[R_TexFormat_R8]    = GL_RED,
			[R_TexFormat_RG8]   = GL_RG,
			[R_TexFormat_RGB8]  = GL_RGB,
			[R_TexFormat_RGBA8] = GL_RGBA,
		};
		GLenum format = fmt_map[tex->fmt];
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, (void *) data);
	}
}

function void *
r_ogl_os_load_procedure(const char *name)
{
  void *result = (void *) glXGetProcAddressARB((u8 *) name);
  return result;
}

function GLuint 
r_ogl_compile_shader(String8 vert, String8 frag)
{
	GLuint vs_id = glCreateShader(GL_VERTEX_SHADER);
	char *vs_src = (char *)vert.raw;
	s64 vs_len = vert.len;
	glShaderSource(vs_id, 1, &vs_src, (int*)&vs_len);
	glCompileShader(vs_id);

	GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);
	char *fs_src = (char *)frag.raw;
	s64 fs_len = frag.len;
	glShaderSource(fs_id, 1, &fs_src, (int*)&fs_len);
	glCompileShader(fs_id);

	GLuint program = glCreateProgram();
	glAttachShader(program, vs_id);
	glAttachShader(program, fs_id);
	glLinkProgram(program);

	glDeleteShader(vs_id);
	glDeleteShader(fs_id);

	return program;
}

function GLuint 
r_ogl_instance_buffer_from_size(u64 size)
{
	GLuint buf = r_ogl_state->scratch_vbo;
	if(size > QUAD_BATCH_SIZE * sizeof(R_Quad))
	{
		glGenBuffers(1, &buf);
		glBindBuffer(GL_ARRAY_BUFFER, buf);
		glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
		R_OGL_Buffer *node = arena_push(r_frame_arena(), R_OGL_Buffer, 1);
		node->id = buf;
		SLLStackPush(r_ogl_state->flush_buffers, node);
	}
	return buf;
}

function void 
r_ogl_target_generate(R_OGL_Target *t, s32 w, s32 h)
{
	if(t->fbo && t->w == w && t->h == h) 
	{
		return;
	}

	if(!t->fbo) 
	{ 
		glGenFramebuffers(1, &t->fbo); 
		glGenTextures(1, &t->tex); 
	}

	glBindTexture(GL_TEXTURE_2D, t->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->tex, 0);

	t->w = w;
	t->h = h;
}

function void 
r_ogl_draw_instances(GLuint shader, void *data, s64 count, s64 stride, R_OGL_Attr *attrs, s64 attr_count, R_Handle tex_h)
{
	s64 bytes = count * stride;
	R_OGL_Texture *tex = (R_OGL_Texture *) tex_h.v[0];
	if(tex == 0) tex = (R_OGL_Texture *) r_ogl_state->white_tex.v[0];

	glUseProgramScope(shader)
	{
		GLuint buf = r_ogl_instance_buffer_from_size(bytes);
		glBindVertexArrayScope(r_ogl_state->gen_purpose_vao)
		glBindBufferScope(GL_ARRAY_BUFFER, buf)
		{
			glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, data);
			for EachIndex(i, attr_count)
			{
				glEnableVertexAttribArray(i);
				glVertexAttribPointer(i, attrs[i].n, GL_FLOAT, GL_FALSE, (GLsizei)stride, (void *) attrs[i].off);
				glVertexAttribDivisor(i, 1);
			}
	
			glUniform1i(glGetUniformLocation(shader, "tex"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTextureScope(GL_TEXTURE_2D, tex->id)
			{
				glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)count);
			}
		}
	}
}

function void 
r_ogl_set_scissor(Rng2S32 rect)
{
	s32 h = r_ogl_state->targets[0].h;
	glScissor(rect.min.x,
	          h - rect.max.y,
	          rect.max.x - rect.min.x,
	          rect.max.y - rect.min.y);
}
