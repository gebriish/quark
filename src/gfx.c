#include "gfx.h"

#include "thirdparty/glad/glad.h"
#include "thirdparty/glad/glad.c"

global GFX_Context *g_ctx;


internal bool rect_vs_rect(Rect r1, Rect r2)
{
    if (r1.to.x   <= r2.from.x) return false;
    if (r1.from.x >= r2.to.x)   return false;

    if (r1.to.y   <= r2.from.y) return false;
    if (r1.from.y >= r2.to.y)   return false;
	return true;
}


/////////////////////////////////////////////
// ~geb: shader source

global const String8 quad_shader_src = S(
	"#vs\n"
	"#version 330 core\n"

	"layout (location = 0) in vec2 a_pos;\n"
	"layout (location = 1) in vec4 a_color;\n"
	"layout (location = 2) in vec2 a_texcoord;\n"
	"layout (location = 3) in vec2 a_circcoord;\n"

	"uniform mat4 u_proj;\n"

	"out vec4 f_color;\n"
	"out vec2 f_texcoord;\n"
	"out vec2 f_circcoord;\n"


	"void main() {\n"
	"\tf_color = a_color;\n"
	"\tf_texcoord = a_texcoord;\n"
	"\tf_circcoord = a_circcoord;\n"
	"\tgl_Position = u_proj * vec4(a_pos.xy, 0.0, 1.0);\n"
	"}\n"

	"#fs\n"
	"#version 330 core\n"

	"uniform sampler2D u_texture;\n"

	"in vec4 f_color;\n"
	"in vec2 f_texcoord;\n"
	"in vec2 f_circcoord;\n"

	"out vec4 frag_color;\n"

	"void main() {\n"
	"\tfloat dist = length((f_circcoord - 0.5) * 2)-1.0;\n"
	"\tfloat edge_width = fwidth(dist)*0.5;\n"
	"\tfloat alpha = 1.0-smoothstep(-edge_width,edge_width,dist);\n"
	"\tfrag_color = texture(u_texture, f_texcoord) * f_color * vec4(vec3(1.0), alpha);\n"
	"}\n"
);


///////////////////
// ~geb : helper functions


internal Shader_Program
compile_shader_program(String8 vtx_source, String8 frg_source)
{
	u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, (const char**)&vtx_source.str, 
				(int*)&vtx_source.len);
	glCompileShader(vertex_shader);

	int success;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
		printf("Vertex shader compilation failed: %s\n", info_log);
	}

	u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(
		fragment_shader, 
		1, 
		cast(const char**)&frg_source.str, 
		cast(int*)&frg_source.len
	);
	glCompileShader(fragment_shader);
	
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
	   char info_log[512];
	   glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
	   printf("Fragment shader compilation failed: %s\n", info_log);
	}

	Shader_Program program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetProgramInfoLog(program, 512, NULL, info_log);
		printf("Shader program linking failed: %s\n", info_log);
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}


internal void
_resize_proc(Window_Handle window, i32 w, i32 h)
{
	glViewport(0, 0, w, h);
	f32 proj[16] = {
			2.0f / (f32)w, 0.0f, 0.0f, 0.0f,
			0.0f, -2.0f / (f32)h, 0.0f, 0.0f,
			0.0f, 0.0f, -1.0f, 0.0f,
			-1.0f, 1.0f, 0.0f, 1.0f};

	glUniformMatrix4fv(g_ctx->quad_uniforms[Uniform_Proj], 1, false, proj);
	g_ctx->resolution.x = w;
	g_ctx->resolution.y = h;
}

internal void
_prepare_batch(u32 tex_id)
{
	if (g_ctx->active_texture != tex_id) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_id);
		g_ctx->active_texture = tex_id;
	}
	g_ctx->render_batch.vertex_count = 0;
	g_ctx->render_batch.index_count  = 0;
}

internal void
_flush_batch()
{
	glBufferSubData(
		GL_ARRAY_BUFFER,
		0,
		g_ctx->render_batch.vertex_count * VTX_SIZE,
		g_ctx->render_batch.vertices
	);
	glBufferSubData(
		GL_ELEMENT_ARRAY_BUFFER,
		0,
		g_ctx->render_batch.index_count * sizeof(u16),
		g_ctx->render_batch.indices
	);

	glDrawElements(
		GL_TRIANGLES,
		(int)(g_ctx->render_batch.index_count),
		GL_UNSIGNED_SHORT,
		NULL
	);
}

internal void
_batch_flush_if_needed(u32 needed_vertices, u32 needed_indices, u32 tex_id)
{
    if (g_ctx->active_texture != tex_id) {
        if (g_ctx->render_batch.index_count > 0) {
            _flush_batch();
        }
        _prepare_batch(tex_id);
        return;
    }

    if (g_ctx->render_batch.vertex_count + needed_vertices > MAX_VERTEX_COUNT ||
        g_ctx->render_batch.index_count  + needed_indices  > MAX_VERTEX_COUNT)
    {
        _flush_batch();
        _prepare_batch(tex_id);
    }
}

///////////////////

internal GFX_Context
gfx_make(String8 title_cstring, i32 w, i32 h, Allocator allocator, Allocator temp_allocator)
{
	GFX_Context state = {0};

	state.allocator = allocator;
	state.temp_allocator = temp_allocator;

	RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
	hints->major = 3;
	hints->minor = 3;
	RGFW_setGlobalHints_OpenGL(hints);

	state.window = RGFW_createWindow(
		(const char *) title_cstring.str, 0, 0, w, h,
		RGFW_windowAllowDND | RGFW_windowCenter | RGFW_windowScaleToMonitor | RGFW_windowOpenGL
	);

	RGFW_window_swapInterval_OpenGL(state.window, 1);

	gfx_set_context(&state);

	{
		u32 data = 0xffffffff;
		Image write_image = {
			.width     = 1,
			.height    = 1,
			.pixel_fmt = Pixel_RGBA8,
			.data      = cast(u8 *) &data,
		};
		u32 t_id = gfx_texture_upload(write_image, TextureKind_Normal);
		Assert(t_id == 1);
	}

	{
		state.quad_shader = gfx_compile_program(quad_shader_src);

		const String8 uniform_strings[Uniform_Count] = {
			[Uniform_Proj]    = S("u_proj"),
			[Uniform_Texture] = S("u_texture"),
		};

		gfx_shader_load_uniforms(
			state.quad_shader,
			state.quad_uniforms,
			Uniform_Count,
			uniform_strings
		);

		glUseProgram(state.quad_shader);
		glUniform1i(state.quad_uniforms[Uniform_Texture], 0);
	}

	{ // render batch setup
		glGenVertexArrays(1, &state.batch_vao);
		glBindVertexArray(state.batch_vao);

		glGenBuffers(1, &state.batch_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, state.batch_vbo);
		glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_COUNT * VTX_SIZE, NULL, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, false, VTX_SIZE, (void *)OffsetOf(Vertex_2D, position));
		glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, VTX_SIZE, (void *)OffsetOf(Vertex_2D, color));
		glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, true, VTX_SIZE, (void *)OffsetOf(Vertex_2D, texcoords));
		glVertexAttribPointer(3, 2, GL_UNSIGNED_SHORT, true, VTX_SIZE, (void *)OffsetOf(Vertex_2D, circle_mask_coord));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);

		glGenBuffers(1, &state.batch_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.batch_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_VERTEX_COUNT * sizeof(u16), NULL, GL_DYNAMIC_DRAW);
	
		Alloc_Error err = 0;
		state.render_batch.vertices = alloc_array(allocator, Vertex_2D, MAX_VERTEX_COUNT, &err);
		if (err) {
			log_error("Failed to allocate vertex array, error(%d)", err); Trap();
		}

		state.render_batch.indices  = alloc_array(allocator, u16,     MAX_VERTEX_COUNT, &err);
		if (err) {
			log_error("Failed to allocate index array, error(%d)", err); Trap();
		}
	}

	_resize_proc(state.window, w, h);

	state.last_frame_time = os_time_now();
	return state;
}

internal GFX_Context *
gfx_set_context(GFX_Context *ctx)
{
	RGFW_window_makeCurrentContext_OpenGL(ctx->window);

	if (!gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL)) {
		printf("Failed to initialize GLAD\n");
		Trap();
	}

	GFX_Context *prev = g_ctx;
	g_ctx = ctx;

	///////////////////
	// ~geb: pipeline initialization

	RGFW_window_setUserPtr(g_ctx->window, cast(void *)g_ctx);
	RGFW_setWindowResizedCallback(_resize_proc);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return prev;
}

internal bool
gfx_window_open()
{
	Assert(g_ctx);
	return !RGFW_window_shouldClose(g_ctx->window);
}

internal void
gfx_mouse_position(f32 *x, f32 *y)
{
	*x = cast(f32) g_ctx->mouse_x;
	*y = cast(f32) g_ctx->mouse_y;
}

internal f64
gfx_delta_time() {
	return g_ctx->frame_delta;
}

internal Rect
gfx_get_clip_rect()
{
	Rect r = {
		{0, 0},
		{(f32)g_ctx->resolution.x, (f32)g_ctx->resolution.y}
	};
	return r;
}

internal Frame_Input
gfx_frame_begin(color8_t col)
{
	OS_Time_Stamp curr_time = os_time_now();
	g_ctx->frame_delta = os_time_diff(g_ctx->last_frame_time, curr_time).seconds;
	g_ctx->last_frame_time = curr_time;	

	//RGFW_waitForEvent(RGFW_eventWaitNext);

	Frame_Input input_data = {0};

	RGFW_event event = {0};
	while(RGFW_window_checkEvent(g_ctx->window, &event)) {
		switch(event.type) {
			case RGFW_mousePosChanged:
				g_ctx->mouse_x = event.mouse.x;
				g_ctx->mouse_y = event.mouse.y;
				break;
			case RGFW_keyPressed:
				MaskSet( input_data.special_key_presses, event.key.value == RGFW_backSpace, Pressed_Backspace);
				MaskSet( input_data.special_key_presses, event.key.value == RGFW_enter, Pressed_Enter);
				MaskSet( input_data.special_key_presses, event.key.value == RGFW_delete, Pressed_Delete);
				MaskSet( input_data.special_key_presses, event.key.value == RGFW_left, Pressed_Move_Left);
				MaskSet( input_data.special_key_presses, event.key.value == RGFW_right, Pressed_Move_Right);
				MaskSet( input_data.special_key_presses, event.key.value == RGFW_up, Pressed_Move_Up);
				MaskSet( input_data.special_key_presses, event.key.value == RGFW_down, Pressed_Move_Down);

				if (RGFW_isKeyDown(RGFW_space)) {
					u8 *str = alloc_array_nz(g_ctx->temp_allocator, u8, 1, NULL);
					input_data.text = str8(str, 1);
					*input_data.text.str = cast(u8) ' ';
				} else if (RGFW_isKeyDown(RGFW_tab)) {
					u8 *str = alloc_array_nz(g_ctx->temp_allocator, u8, 1, NULL);
					input_data.text = str8(str, 1);
					*input_data.text.str = cast(u8) '\t';
				} else if (!input_data.special_key_presses && event.key.sym) {
					u8 *str = alloc_array_nz(g_ctx->temp_allocator, u8, 1, NULL);
					input_data.text = str8(str, 1);
					*input_data.text.str = cast(u8) event.key.sym;
				}

				break;
			case RGFW_mouseScroll:
				input_data.scroll_x = event.scroll.x;
				input_data.scroll_y = event.scroll.y;
				break;
		}
	}


	glUseProgram(g_ctx->quad_shader);

	glBindVertexArray(g_ctx->batch_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_ctx->batch_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ctx->batch_ebo);

	glClearColor(color_r(col)/255.0, color_g(col)/255.0, color_b(col)/255.0, color_a(col)/255.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_prepare_batch(WHITE_TEXTURE);
	return input_data;
}

internal void
gfx_frame_end()
{
	_flush_batch();
	RGFW_window_swapBuffers_OpenGL(g_ctx->window);
}

internal Shader_Program
gfx_compile_program(String8 content)
{
	String8 vertex_source = {0};
	String8 fragment_source = {0};

	u8 *ptr = content.str;
	u8 *end = content.str + content.len;

	while (ptr < end) {
		if (ptr + 3 <= end && MemCompare(ptr, "#vs", 3) == 0) {
			while (ptr < end && *ptr != '\n') ptr++;
			if (ptr < end) ptr++;

			u8 *vs_start = ptr;

			while (ptr < end) {
				if (ptr + 3 <= end && MemCompare(ptr, "#fs", 3) == 0) {
					break;
				}
				ptr++;
			}

			vertex_source.str =vs_start;
			vertex_source.len = ptr - vs_start;
			break;
		}
		ptr++;
	}

	ptr = content.str;
	while (ptr < end) {
		if (ptr + 3 <= end && MemCompare(ptr, "#fs", 3) == 0) {
			while (ptr < end && *ptr != '\n') ptr++;
			if (ptr < end) ptr++;

			u8 *fs_start = ptr;

			while (ptr < end) {
				if (ptr + 3 <= end && MemCompare(ptr, "#vs", 3) == 0) {
					break;
				}
				ptr++;
			}

			fragment_source.str = fs_start;
			fragment_source.len = ptr - fs_start;
			break;
		}
		ptr++;
	}

	Shader_Program program = compile_shader_program(vertex_source, fragment_source);
	return program;
}

internal void
gfx_delete_program(Shader_Program program)
{
	glDeleteProgram(program);
}


internal u32
gfx_texture_upload(Image data, Texture_Kind type)
{
	u32 gl_id = 0;

	glGenTextures(1, &gl_id);

	i32 internal_format;
	u32 format;
	switch (data.pixel_fmt)
	{
		case Pixel_R8:
			internal_format = GL_RED;
			format = GL_RED;
			break;
		case Pixel_RG8:
			internal_format = GL_RG;
			format = GL_RG;
			break;
		case Pixel_RGB8:
			internal_format = GL_RGB;
			format = GL_RGB;
			break;
		case Pixel_RGBA8:
			internal_format = GL_RGBA;
			format = GL_RGBA;
			break;
		default:
			glDeleteTextures(1, &gl_id);
			return 0;
	};

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (type == TextureKind_GreyScale && data.pixel_fmt == Pixel_R8)
	{
		i32 swizzle[4] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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

	return gl_id;
}


internal void
gfx_texture_unload(u32 tex_id)
{
	glDeleteTextures(1, &tex_id);
}

internal void
gfx_texture_sub_data(u32 tex_id, i32 x, i32 y, Image img)
{
	i32 internal_format;
	u32 format;
	switch (img.pixel_fmt)
	{
		case Pixel_R8:
			internal_format = GL_RED;
			format = GL_RED;
			break;
		case Pixel_RG8:
			internal_format = GL_RG;
			format = GL_RG;
			break;
		case Pixel_RGB8:
			internal_format = GL_RGB;
			format = GL_RGB;
			break;
		case Pixel_RGBA8:
			internal_format = GL_RGBA;
			format = GL_RGBA;
			break;
		default:
			return;
	};

	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		x,
		y,
		img.width,
		img.height,
		format,
		GL_UNSIGNED_BYTE,
		img.data
	);
}


internal void
gfx_push_rect(vec2 pos, vec2 size, color8_t color, u32 texture, Rect tex_coords, Rect circle_coords)
{
	Assert(g_ctx);

	color = ByteSwapU32(color);

	_batch_flush_if_needed(4, 6, texture);

	u32 base = g_ctx->render_batch.vertex_count;

	Vertex_2D *v = cast(Vertex_2D *)g_ctx->render_batch.vertices + base;
	u16 *i = g_ctx->render_batch.indices + g_ctx->render_batch.index_count;

	f32 x1 = pos.x + size.x;
	f32 y1 = pos.y + size.y;

	u16 tu0 = cast(u16)(tex_coords.from.x * cast(f32)U16_MAX);
	u16 tv0 = cast(u16)(tex_coords.from.y * cast(f32)U16_MAX);
	u16 tu1 = cast(u16)(tex_coords.to.x * cast(f32)U16_MAX);
	u16 tv1 = cast(u16)(tex_coords.to.y * cast(f32)U16_MAX);

	u16 cu0 = cast(u16)(circle_coords.from.x * cast(f32)U16_MAX);
	u16 cv0 = cast(u16)(circle_coords.from.y * cast(f32)U16_MAX);
	u16 cu1 = cast(u16)(circle_coords.to.x * cast(f32)U16_MAX);
	u16 cv1 = cast(u16)(circle_coords.to.y * cast(f32)U16_MAX);

	u16 cm = U16_MAX / 2;

	v[0].position.x = pos.x;   v[0].position.y = pos.y;
	v[1].position.x = x1;      v[1].position.y = pos.y;
	v[2].position.x = x1;      v[2].position.y = y1;
	v[3].position.x = pos.x;   v[3].position.y = y1;

	v[0].texcoords.x = tu0; v[0].texcoords.y = tv0;
	v[1].texcoords.x = tu1; v[1].texcoords.y = tv0;
	v[2].texcoords.x = tu1; v[2].texcoords.y = tv1;
	v[3].texcoords.x = tu0; v[3].texcoords.y = tv1;

	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;

	v[0].circle_mask_coord.x = cu0; v[0].circle_mask_coord.y = cv0;
	v[1].circle_mask_coord.x = cu1; v[1].circle_mask_coord.y = cv0;
	v[2].circle_mask_coord.x = cu1; v[2].circle_mask_coord.y = cv1;
	v[3].circle_mask_coord.x = cu0; v[3].circle_mask_coord.y = cv1;

	i[0] = base + 0;
	i[1] = base + 1;
	i[2] = base + 2;
	i[3] = base + 2;
	i[4] = base + 3;
	i[5] = base + 0;

	g_ctx->render_batch.vertex_count += 4;
	g_ctx->render_batch.index_count  += 6;
}
