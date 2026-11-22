
function void
rndr_init(void)
{
  // setting up the opengl context
  opengl_os_init();

  // loading opengl procedures from os
#define X(name, r, p) name = (name##_FunctionType *)opengl_os_load_procedure(#name);
  Opengl_Proc_List
#undef X

  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,   // color
                      GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);  // alpha

  // internal opengl renderer state setup.
  Arena *arena = arena_reserve(Megabytes(4));
  Arena *frame = arena_reserve(Megabytes(4));

  opengl_state = arena_push(arena, Opengl_State, 1);
  opengl_state->arena = arena;
  opengl_state->frame_arena = frame;

  // setup quad batch buffer

  {
    glGenVertexArrays(1, &opengl_state->gen_purpose_vao);

    glGenBuffers(1, &opengl_state->scratch_vbo);
    glBindBufferScope(GL_ARRAY_BUFFER, opengl_state->scratch_vbo)
    {
      glBufferData(GL_ARRAY_BUFFER, QUAD_BATCH_SIZE * sizeof(Quad_Inst), 0, GL_DYNAMIC_DRAW);
    }
  }

  // compile all shaders
  {
    struct { String8 vert; String8 frag; } 
    shaders_src[] = 
    {
      {opengl_quad_vertex_glsl,       opengl_quad_fragment_glsl},
      {opengl_blur_vertex_glsl,       opengl_blur_fragment_glsl},
      {opengl_text_vertex_glsl,       opengl_text_fragment_glsl},
    };

    for each_index(i, CArrayLength(shaders_src))
    {
      opengl_state->shaders[i] = opengl_compile_shader(shaders_src[i].vert, shaders_src[i].frag);
    }
  }

  // texture initialization
  {
    u32 white = 0xffffffff;
    Image white_img = {
      .data = (u8 *) &white,
      .dim = { 1, 1 },
      .fmt = Pixel_Format_RGBA8,
    };
    opengl_state->white_tex = texture_alloc(white_img);
  }

  fnt_init();
}

function void 
window_frame_begin(Win_Handle win)
{
  opengl_select_window(win);

  Vec2F32 size = window_get_size(win);

  opengl_target_generate(&opengl_state->targets[0], (s32)size.x, (s32)size.y);
  opengl_target_generate(&opengl_state->targets[1], (s32)size.x, (s32)size.y);

  glBindFramebuffer(GL_FRAMEBUFFER, opengl_state->targets[0].fbo);
  glViewport(0, 0, (s32)size.x, (s32)size.y);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  Rng3F32 view_box = {
    .min = v3f32(0,size.y,-1),
    .max = v3f32(size.x,0, 1),
  };
  Mat4x4F32 proj = mat4_ortho(view_box);

  for each_index(i, OGL_Shader_Kind_COUNT)
  {
    glUseProgramScope(opengl_state->shaders[i])
    {
      GLint proj_loc = glGetUniformLocation(opengl_state->shaders[i], "projection");
      glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.raw);
    }
  }
}

function void 
window_frame_end(Win_Handle win)
{
  Vec2F32 size = window_get_size(win);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, opengl_state->targets[0].fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, (s32)size.x, (s32)size.y, 0, 0, (s32)size.x, (s32)size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  opengl_window_swap_buffers(win);
}

function void 
submit_draw_list(Draw_List list)
{
  Temp scratch = scratch_begin(0, 0);
  glEnable(GL_SCISSOR_TEST);

  Clip_Rect *clip_stack = arena_push(scratch.arena, Clip_Rect, 1);
  clip_stack->rect.max.x = opengl_state->targets[0].w;
  clip_stack->rect.max.y = opengl_state->targets[0].h;
  opengl_set_scissor(clip_stack->rect);

  for each_node(draw, Draw_Call, list.first) switch (draw->kind)
  {
    case Draw_Kind_ClipPush:
    {
      Clip_Rect *node = arena_push(scratch.arena, Clip_Rect, 1);
      node->rect = draw->rect;
      SLLStackPush(clip_stack, node);
      opengl_set_scissor(node->rect);
    } break;

    case Draw_Kind_ClipPop:
    {
      SLLStackPop(clip_stack);
      opengl_set_scissor(clip_stack->rect);
    } break;

    case Draw_Kind_Quads:
    {
      Quad_Batch *b = &draw->quad_batch;
      if(b->len == 0) break;
      OGL_Attr attrs[] = 
      {
        {4, offsetof(Quad_Inst, dst)      }, {4, offsetof(Quad_Inst, src)      },
        {4, offsetof(Quad_Inst, col)      }, {4, offsetof(Quad_Inst, radii)    },
        {1, offsetof(Quad_Inst, thickness)}, {1, offsetof(Quad_Inst, edge_blur)},
      };
      opengl_draw_instances(opengl_state->shaders[OGL_Shader_Kind_Quad], b->raw, b->len, sizeof(Quad_Inst), attrs, CArrayLength(attrs), b->tex);
    } break;

    case Draw_Kind_Text:
    {
      Text_Batch *b = &draw->text_batch;
      if(b->len == 0) break;

      GLuint shader = opengl_state->shaders[OGL_Shader_Kind_Text];
      glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC1_COLOR, 
                          GL_ONE, GL_ONE_MINUS_SRC1_ALPHA);

      OGL_Attr attrs[] =
      {
        {4, offsetof(Quad_Text, dst)},
        {4, offsetof(Quad_Text, src)},
        {4, offsetof(Quad_Text, fg)},
      };
      opengl_draw_instances(shader, b->raw, b->len, sizeof(Quad_Text), attrs, CArrayLength(attrs), b->tex);

      glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, 
                          GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);
    } break;

    case Draw_Kind_Blur:
    {
      Quad_Blur *params = &draw->blur;
      Vec2F32 viewport_dim = v2f32((f32) opengl_state->targets[0].w, (f32) opengl_state->targets[0].h);
      GLuint shader = opengl_state->shaders[OGL_Shader_Kind_Blur];

      glUseProgramScope(shader) glBindVertexArrayScope(opengl_state->gen_purpose_vao)
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
        glBindFramebuffer(GL_FRAMEBUFFER, opengl_state->targets[1].fbo);
        glBindTextureScope(GL_TEXTURE_2D, opengl_state->targets[0].tex)
        {
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glUniform2f(glGetUniformLocation(shader, "direction"), 1.f/viewport_dim.x, 0);
          glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // second pass
        glBindFramebuffer(GL_FRAMEBUFFER, opengl_state->targets[0].fbo);
        glBindTextureScope(GL_TEXTURE_2D, opengl_state->targets[1].tex)
        {
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glUniform2f(glGetUniformLocation(shader, "direction"), 0, 1.f/viewport_dim.y);
          glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
      }
    } break;
  }

  for each_node(buf, OGL_Buffer, opengl_state->flush_buffers)
  {
    glDeleteBuffers(1, &buf->id);
  }

  glDisable(GL_SCISSOR_TEST);
  scratch_end(scratch);
}

function Arena *
frame_arena(void)
{
  return opengl_state->frame_arena;
}

function void 
frame_end(void)
{
  fnt_frame();
  arena_free(opengl_state->frame_arena, false);
}

function Rndr_Handle 
texture_alloc(Image image)
{
  OGL_Texture *tex = opengl_state->texture_free;
  if (opengl_state->texture_free)
  {
    SLLStackPop(opengl_state->texture_free);
  }
  else
  {
    tex = arena_push_non_zero(opengl_state->arena, OGL_Texture, 1);
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
      [Pixel_Format_R8]    = { GL_R8,    GL_RED  },
      [Pixel_Format_RG8]   = { GL_RG8,   GL_RG   },
      [Pixel_Format_RGB8]  = { GL_RGB8,  GL_RGB  },
      [Pixel_Format_RGBA8] = { GL_RGBA8, GL_RGBA },
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

  Rndr_Handle result = {0};
  result.v[0] = (u64) tex;
  return result;
}

function Vec2S32
texture_size(Rndr_Handle h)
{
	OGL_Texture *tex = (OGL_Texture *) h.v[0];
	return tex->dim;
}

function void
texture_free(Rndr_Handle h)
{
	OGL_Texture *t = (OGL_Texture *)h.v[0];
	if (t != 0 && t->id != 0)
	{
		glDeleteTextures(1, &t->id);
		SLLStackPush(opengl_state->texture_free, t);
	}
}

function void
texture_update(Rndr_Handle h, Rng2S32 range, u8 *data)
{
	OGL_Texture *tex = (OGL_Texture *) h.v[0];

	s32 x = range.begin[0];
	s32 y = range.begin[1];

	s32 width  = range.end[0] - x;
	s32 height = range.end[1] - y;

	glBindTextureScope(GL_TEXTURE_2D, tex->id)
	{
		GLenum fmt_map[] = 
		{
			[Pixel_Format_R8]    = GL_RED,
			[Pixel_Format_RG8]   = GL_RG,
			[Pixel_Format_RGB8]  = GL_RGB,
			[Pixel_Format_RGBA8] = GL_RGBA,
		};
		GLenum format = fmt_map[tex->fmt];
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, (void *) data);
	}
}

function void *
opengl_os_load_procedure(const char *name)
{
  void *result = (void *) glXGetProcAddressARB((u8 *) name);
  return result;
}

function GLuint 
opengl_compile_shader(String8 vert, String8 frag)
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
opengl_instance_buffer_from_size(u64 size)
{
	GLuint buf = opengl_state->scratch_vbo;
	if(size > QUAD_BATCH_SIZE * sizeof(Quad_Inst))
	{
		glGenBuffers(1, &buf);
		glBindBuffer(GL_ARRAY_BUFFER, buf);
		glBufferData(GL_ARRAY_BUFFER, (ptrdiff_t) size, 0, GL_DYNAMIC_DRAW);
		OGL_Buffer *node = arena_push(frame_arena(), OGL_Buffer, 1);
		node->id = buf;
		SLLStackPush(opengl_state->flush_buffers, node);
	}
	return buf;
}

function void 
opengl_target_generate(OGL_Target *t, s32 w, s32 h)
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
opengl_draw_instances(GLuint shader, void *data, s64 count, s64 stride, OGL_Attr *attrs, s64 attr_count, Rndr_Handle tex_h)
{
	s64 bytes = count * stride;
	OGL_Texture *tex = (OGL_Texture *) tex_h.v[0];
	if(tex == 0) tex = (OGL_Texture *) opengl_state->white_tex.v[0];

	glUseProgramScope(shader)
	{
		GLuint buf = opengl_instance_buffer_from_size((u64) bytes);
		glBindVertexArrayScope(opengl_state->gen_purpose_vao)
		glBindBufferScope(GL_ARRAY_BUFFER, buf)
		{
			glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, data);
			for each_index(i, attr_count)
			{
				glEnableVertexAttribArray((GLuint) i);
				glVertexAttribPointer((GLuint) i, attrs[i].n, GL_FLOAT, GL_FALSE, (GLsizei)stride, (void *) attrs[i].off);
				glVertexAttribDivisor((GLuint) i, 1);
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
opengl_set_scissor(Rng2S32 rect)
{
	s32 h = opengl_state->targets[0].h;
	glScissor(rect.min.x,
	          h - rect.max.y,
	          rect.max.x - rect.min.x,
	          rect.max.y - rect.min.y);
}
