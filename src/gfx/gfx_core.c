#include "gfx_core.h"
#include "gfx_font.h"

#include "../os/os_core.h"

#include "../thirdparty/glad/glad.c"

#define VTX_SIZE sizeof(Render_Vertex)

global GFX_State *_gfx_state = NULL;

global String8 VERTEX_SHADER_SRC = S(
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
		"}");

global String8 FRAGMENT_SHADER_SRC = S(
		"#version 330 core\n"
		"in vec2 Frag_UV;"
		"in vec2 Frag_CircCoords;"
		"in vec4 Frag_Color;"
		"in float Frag_TexID;"

		"uniform sampler2D Textures[16];"

		"layout(location=0) out vec4 Out_Color;"

		"void main(){"
		"int tex_id=int(Frag_TexID);"
		"float dist=length(Frag_CircCoords)-1.0;"
		"float edge_width=fwidth(dist)*0.5;"
		"float alpha=1.0-smoothstep(-edge_width,edge_width,dist);"

		"vec4 texColor=vec4(1.0);"

		"			if(tex_id==0) texColor = texture(Textures[0], Frag_UV);"
		"else if(tex_id==1) texColor = texture(Textures[1], Frag_UV);"
		"else if(tex_id==2) texColor = texture(Textures[2], Frag_UV);"
		"else if(tex_id==3) texColor = texture(Textures[3], Frag_UV);"
		"else if(tex_id==4) texColor = texture(Textures[4], Frag_UV);"
		"else if(tex_id==5) texColor = texture(Textures[5], Frag_UV);"
		"else if(tex_id==6) texColor = texture(Textures[6], Frag_UV);"
		"else if(tex_id==7) texColor = texture(Textures[7], Frag_UV);"
		"else if(tex_id==8) texColor = texture(Textures[8], Frag_UV);"
		"else if(tex_id==9) texColor = texture(Textures[9], Frag_UV);"
		"else if(tex_id==10)texColor = texture(Textures[10],Frag_UV);"
		"else if(tex_id==11)texColor = texture(Textures[11],Frag_UV);"
		"else if(tex_id==12)texColor = texture(Textures[12],Frag_UV);"
		"else if(tex_id==13)texColor = texture(Textures[13],Frag_UV);"
		"else if(tex_id==14)texColor = texture(Textures[14],Frag_UV);"
		"else if(tex_id==15)texColor = texture(Textures[15],Frag_UV);"

		"vec4 base=Frag_Color*texColor;"
		"Out_Color=vec4(base.rgb,base.a*alpha);"
		"}");

internal void
resize_callback(GLFWwindow *window, int w, int h)
{
	gfx_resize_target(w, h);
}

internal void
char_callback(GLFWwindow *window, unsigned int codepoint)
{
	GFX_State *gfx = (GFX_State *) glfwGetWindowUserPointer(window);
	Input_Data *input = &gfx->input_data;

	input->codepoint = codepoint;
}

internal void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	GFX_State *gfx = (GFX_State *) glfwGetWindowUserPointer(window);
	Input_Data *input = &gfx->input_data;

	if (action == GLFW_REPEAT)
		input->key_repeat = true;

	if (action != GLFW_RELEASE)
		switch (key) {
			case GLFW_KEY_ENTER:
				input->codepoint = (rune) ASCII_LF;
				return;
			case GLFW_KEY_TAB:
				input->codepoint = (rune) ASCII_HT;
				return;

			case GLFW_KEY_BACKSPACE:
				input->codepoint = (rune) ASCII_BS;
				return;
			case GLFW_KEY_DELETE:
				input->codepoint = (rune) ASCII_DEL;
				return;
		}
}

internal bool
gfx_init(GFX_State *gfx)
{
	Assert(gfx);
	MemZeroStruct(gfx);

	{ // Initialize and Open a GLFWwindow
		if (!glfwInit())
		{
			log_error("Failed to initialize GLFW\n");
			return false;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow *window = glfwCreateWindow(1000, 625, "quark", NULL, NULL);
		if (!window)
		{
			log_error("Failed to create GLFW window\n");
			glfwTerminate();
			return false;
		}

		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		glfwSetFramebufferSizeCallback(window, resize_callback);
		glfwSetCharCallback(window, char_callback);
		glfwSetKeyCallback(window, key_callback);

		gfx->glfw_window = window;

		glfwSetWindowUserPointer(window, (void *)gfx);
	}

	gfx->persist_arena = arena_new(MB(4), os_reserve, os_commit, os_decommit, os_release);

	Render_Vertex *vertex_buffer = arena_push_array(gfx->persist_arena, Render_Vertex, MAX_VERTEX_COUNT);
	u16 *index_buffer = arena_push_array(gfx->persist_arena, u16, MAX_VERTEX_COUNT);

	if (!vertex_buffer)
	{
		log_error("Failed to allocate Vertex buffer memory");
		Trap();
	}
	if (!index_buffer)
	{
		log_error("Failed to allocate Index buffer memory");
		Trap();
	}

	gfx->render_buffer.vertices = vertex_buffer;
	gfx->render_buffer.indices = index_buffer;

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		log_error("Failed to initialize GLAD");
		return -1;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_MULTISAMPLE);

	glGenVertexArrays(1, &gfx->vao);
	glBindVertexArray(gfx->vao);

	glGenBuffers(1, &gfx->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo);
	glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_COUNT * VTX_SIZE, NULL, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, false, VTX_SIZE, (void *)OffsetOf(Render_Vertex, position));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, VTX_SIZE, (void *)OffsetOf(Render_Vertex, color));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, true, VTX_SIZE, (void *)OffsetOf(Render_Vertex, uv));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 2, GL_BYTE, true, VTX_SIZE, (void *)OffsetOf(Render_Vertex, circ_mask));
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(4, 1, GL_UNSIGNED_BYTE, false, VTX_SIZE, (void *)OffsetOf(Render_Vertex, tex_id));
	glEnableVertexAttribArray(4);

	glGenBuffers(1, &gfx->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_VERTEX_COUNT * sizeof(u16), NULL, GL_DYNAMIC_DRAW);

	gfx->program = glCreateProgram();

	u32 vtx_shader = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(vtx_shader, 1, (const GLchar *const *)&VERTEX_SHADER_SRC.str, NULL);
	glCompileShader(vtx_shader);

	i32 success;
	char infoLog[512];
	glGetShaderiv(vtx_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vtx_shader, 512, NULL, infoLog);
		log_trace("Vertex Shader compilation Failed\n%s\n", infoLog);
		return false;
	}

	u32 frg_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frg_shader, 1, (const GLchar *const *)&FRAGMENT_SHADER_SRC.str, NULL);
	glCompileShader(frg_shader);

	glGetShaderiv(frg_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(frg_shader, 512, NULL, infoLog);
		log_trace("Fragment Shader Compilation Failed\n%s\n", infoLog);
		return false;
	}

	glAttachShader(gfx->program, vtx_shader);
	glAttachShader(gfx->program, frg_shader);
	glLinkProgram(gfx->program);

	glGetProgramiv(gfx->program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(gfx->program, 512, NULL, infoLog);
		log_trace("Shader program Linking failed\n%s\n", infoLog);
		return false;
	}

	glDeleteShader(frg_shader);
	glDeleteShader(vtx_shader);

	glUseProgram(gfx->program);

	gfx->uniform_loc[Uniform_Textures] = glGetUniformLocation(gfx->program, "Textures");
	gfx->uniform_loc[Uniform_ProjMatrix] = glGetUniformLocation(gfx->program, "ProjMatrix");

	i32 samplers[MAX_TEXTURES] = {0};
	for (i32 i = 0; i < MAX_TEXTURES; i++)
	{
		samplers[i] = i;
	}

	gfx->scissor_stack_len = 0;
	gfx->scissor_enabled = false;

	_gfx_state = gfx;
	glUniform1iv(gfx->uniform_loc[Uniform_Textures], MAX_TEXTURES, samplers);

	u32 white_pixel = 0xffffffff;
	Texture_Data data_white = {
			.data = (u8 *)&white_pixel,
			.width = 1,
			.height = 1,
			.channels = 4,
	};

	gfx_texture_upload(data_white, TextureKind_Normal);
	gfx_resize_target(1000, 625);

	String8 file_data = os_data_from_file_path(gfx->persist_arena, S("./res/fonts/JetBrainsMono-Regular.ttf"));
	if (!file_data.len) {
		log_error("Failed to load Font file"); Trap();
	}
	font_atlas_new(gfx->persist_arena, file_data.str, 512, 22, &gfx->font_atlas);
	font_atlas_add_glyphs_from_string(&gfx->font_atlas, FONT_CHARSET);

	return true;
}

internal void
gfx_deinit(void)
{
	if (!_gfx_state)
		return;

	for (u32 i = 0; i < _gfx_state->texture_count; i++)
	{
		Texture *tex = &_gfx_state->texture_slots[i];
		if (tex->gl_id != 0)
		{
			glDeleteTextures(1, &tex->gl_id);
			tex->gl_id = 0;
		}
	}

	_gfx_state->texture_count = 0;
	_gfx_state->texture_freelist = 0;

	glDeleteBuffers(1, &_gfx_state->ibo);
	_gfx_state->ibo = 0;

	glDeleteBuffers(1, &_gfx_state->vbo);
	_gfx_state->vbo = 0;

	glDeleteVertexArrays(1, &_gfx_state->vao);
	_gfx_state->vao = 0;

	glDeleteProgram(_gfx_state->program);
	_gfx_state->program = 0;

	arena_delete(_gfx_state->persist_arena);
	_gfx_state->persist_arena = NULL;

	glfwDestroyWindow(_gfx_state->glfw_window);
	_gfx_state->glfw_window = NULL;

	glfwTerminate();

	_gfx_state = NULL;
}

internal bool 
gfx_window_open()
{
	if (!_gfx_state)
		return false;

	return !glfwWindowShouldClose(_gfx_state->glfw_window);
}

internal vec2_i32 
gfx_window_size()
{
	int x = 0, y = 0;

	glfwGetWindowSize(_gfx_state->glfw_window, &x, &y);
	return (vec2_i32) {x, y};
}


internal Input_Data
gfx_input_data()
{
	return _gfx_state->input_data;
}

internal void
gfx_resize_target(i32 w, i32 h)
{
	glViewport(0, 0, w, h);
	f32 proj[16] = {
			2.0f / (f32)w, 0.0f, 0.0f, 0.0f,
			0.0f, -2.0f / (f32)h, 0.0f, 0.0f,
			0.0f, 0.0f, -1.0f, 0.0f,
			-1.0f, 1.0f, 0.0f, 1.0f};

	glUniformMatrix4fv(_gfx_state->uniform_loc[Uniform_ProjMatrix], 1, false, proj);
	_gfx_state->viewport.w = w;
	_gfx_state->viewport.h = h;

	if (_gfx_state->scissor_enabled && _gfx_state->scissor_stack_len > 0) {
		Scissor_Rect *current = &_gfx_state->scissor_stack[_gfx_state->scissor_stack_len - 1];
		i32 gl_y = h - (current->y + current->h);
		glScissor(current->x, gl_y, current->w, current->h);
	}
}

internal void
gfx_begin_frame(color8_t color)
{
	_gfx_state->input_data = (Input_Data) {0};

	glfwPollEvents();

	glUseProgram(_gfx_state->program);
	glBindVertexArray(_gfx_state->vao);

	glBindBuffer(GL_ARRAY_BUFFER, _gfx_state->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _gfx_state->ibo);

	glClearColor(
			(f32)color_r(color) / 255.0f,
			(f32)color_g(color) / 255.0f,
			(f32)color_b(color) / 255.0f,
			(f32)color_a(color) / 255.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	font_atlas_update(&_gfx_state->font_atlas);
	gfx_prep();
}

internal void
gfx_end_frame()
{
	gfx_flush();
	glfwSwapBuffers(_gfx_state->glfw_window);
}

internal void
gfx_prep()
{
	_gfx_state->render_buffer.vtx_count = 0;
	_gfx_state->render_buffer.idx_count = 0;
}

internal void
gfx_flush()
{
	glBufferSubData(GL_ARRAY_BUFFER, 0, _gfx_state->render_buffer.vtx_count * VTX_SIZE, _gfx_state->render_buffer.vertices);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, _gfx_state->render_buffer.idx_count * sizeof(u16), _gfx_state->render_buffer.indices); // Changed from sizeof(u32)
	glDrawElements(GL_TRIANGLES, (int)(_gfx_state->render_buffer.idx_count), GL_UNSIGNED_SHORT, NULL);
}

internal void
gfx_push_rect(Rect_Params params)
{
	if (_gfx_state->render_buffer.vtx_count + 4 > MAX_VERTEX_COUNT ||
			_gfx_state->render_buffer.idx_count + 6 > MAX_VERTEX_COUNT)
	{
		gfx_flush();
		gfx_prep();
	}

	u16 base_idx = (u16)(_gfx_state->render_buffer.vtx_count);

	Render_Vertex *vtx = _gfx_state->render_buffer.vertices + base_idx;
	u16 *idx = _gfx_state->render_buffer.indices + _gfx_state->render_buffer.idx_count;

	vec2_f32 pos = params.position;
	color8_t color = params.color;
	vec2_f32 size = params.size;
	vec4_u16 uv = {
			(u16)(params.uv.x * U16_MAX),
			(u16)(params.uv.y * U16_MAX),
			(u16)(params.uv.z * U16_MAX),
			(u16)(params.uv.w * U16_MAX),
	};

	f32 x0 = pos.x, y0 = pos.y;
	f32 x1 = pos.x + size.x, y1 = pos.y + size.y;
	color8_t final_color = hex_color(color);

	vtx[0] = (Render_Vertex){
			.position = {x0, y0},
			.color = final_color,
			.uv = {uv.x, uv.y},
			.circ_mask = {0, 0},
			.tex_id = params.tex_id};

	vtx[1] = (Render_Vertex){
			.position = {x1, y0},
			.color = final_color,
			.uv = {uv.z, uv.y},
			.circ_mask = {0, 0},
			.tex_id = params.tex_id};

	vtx[2] = (Render_Vertex){
			.position = {x1, y1},
			.color = final_color,
			.uv = {uv.z, uv.w},
			.circ_mask = {0, 0},
			.tex_id = params.tex_id};

	vtx[3] = (Render_Vertex){
			.position = {x0, y1},
			.color = final_color,
			.uv = {uv.x, uv.w},
			.circ_mask = {0, 0},
			.tex_id = params.tex_id};

	idx[0] = base_idx;
	idx[1] = base_idx + 1;
	idx[2] = base_idx + 2;
	idx[3] = base_idx + 2;
	idx[4] = base_idx + 3;
	idx[5] = base_idx;

	_gfx_state->render_buffer.vtx_count += 4;
	_gfx_state->render_buffer.idx_count += 6;
}

internal void
gfx_push_rect_rounded(Rect_Params params)
{
	if (params.size.x <= 0.0f || params.size.y <= 0.0f)
		return;

	vec2_f32 size = params.size;
	vec2_f32 pos = params.position;
	u8 tex_id = params.tex_id;
	color8_t color = hex_color(params.color);

	f32 inv_width = 1.0f / size.x;
	f32 inv_height = 1.0f / size.y;
	f32 uv_base_x = params.uv.x;
	f32 uv_base_y = params.uv.y;
	f32 uv_scale_x = (params.uv.z - params.uv.x) * inv_width;
	f32 uv_scale_y = (params.uv.w - params.uv.y) * inv_height;

	f32 r[4] = {
			params.radii.top_left,
			params.radii.top_right,
			params.radii.bottom_right,
			params.radii.bottom_left};

	f32 top_sum = r[0] + r[1];
	if (top_sum > size.x)
	{
		f32 scale = size.x / top_sum;
		r[0] *= scale;
		r[1] *= scale;
	}

	f32 bottom_sum = r[3] + r[2];
	if (bottom_sum > size.x)
	{
		f32 scale = size.x / bottom_sum;
		r[3] *= scale;
		r[2] *= scale;
	}

	f32 left_sum = r[0] + r[3];
	if (left_sum > size.y)
	{
		f32 scale = size.y / left_sum;
		r[0] *= scale;
		r[3] *= scale;
	}

	f32 right_sum = r[1] + r[2];
	if (right_sum > size.y)
	{
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

	rounded_count = (u8)((rounded_mask & 1) + ((rounded_mask >> 1) & 1) + ((rounded_mask >> 2) & 1) + ((rounded_mask >> 3) & 1));

	u32 outline_vertices = 4 + rounded_count;
	u32 corner_vertices = rounded_count * 3;
	u32 total_vertices = outline_vertices + corner_vertices;
	u32 total_indices = (outline_vertices - 2) * 3 + rounded_count * 3;

	if (_gfx_state->render_buffer.vtx_count + total_vertices > MAX_VERTEX_COUNT ||
			_gfx_state->render_buffer.idx_count + total_indices > MAX_VERTEX_COUNT)
	{
		gfx_flush();
		gfx_prep();
	}

	u16 base_idx = (u16)_gfx_state->render_buffer.vtx_count;
	Render_Vertex *vtx = &_gfx_state->render_buffer.vertices[base_idx];
	u16 *idx = &_gfx_state->render_buffer.indices[_gfx_state->render_buffer.idx_count];

	local_persist const vec2_f32 cw[4] = {
			{1.0f, 0.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f}, {0.0f, -1.0f}};
	local_persist const vec2_f32 ccw[4] = {
			{0.0f, 1.0f}, {-1.0f, 0.0f}, {0.0f, -1.0f}, {1.0f, 0.0f}};

	f32 right = pos.x + size.x;
	f32 bottom = pos.y + size.y;
	vec2_f32 corners[4] = {
			{pos.x, pos.y},	 // top-left
			{right, pos.y},	 // top-right
			{right, bottom}, // bottom-right
			{pos.x, bottom}	 // bottom-left
	};

#define WRITE_VERTEX(v_ptr, px, py, cmask)                          \
	do                                                                \
	{                                                                 \
		f32 lx = (px) - pos.x;                                          \
		f32 ly = (py) - pos.y;                                          \
		(v_ptr)->position.x = (px);                                     \
		(v_ptr)->position.y = (py);                                     \
		(v_ptr)->uv.x = (u16)((uv_base_x + lx * uv_scale_x) * U16_MAX); \
		(v_ptr)->uv.y = (u16)((uv_base_y + ly * uv_scale_y) * U16_MAX); \
		(v_ptr)->color = color;                                         \
		(v_ptr)->circ_mask = (cmask);                                   \
		(v_ptr)->tex_id = tex_id;                                       \
	} while (0)

	u16 v_idx = 0;

	for (i32 i = 0; i < 4; i++)
	{
		vec2_f32 corner = corners[i];
		f32 radius = r[i];

		if (!(rounded_mask & (1 << i)))
		{
			WRITE_VERTEX(&vtx[v_idx], corner.x, corner.y, ((vec2_i8){0, 0}));
			v_idx++;
		}
		else
		{
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
	for (u16 i = 1; i < outline_vertices - 1; i++)
	{
		idx[idx_count++] = fan_center;
		idx[idx_count++] = (u16)(base_idx + i + 1);
		idx[idx_count++] = (u16)(base_idx + i);
	}

	for (i32 i = 0; i < 4; i++)
	{
		if (!(rounded_mask & (1 << i)))
			continue;

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

	_gfx_state->render_buffer.vtx_count += total_vertices;
	_gfx_state->render_buffer.idx_count += total_indices;
}

internal Text_Metrics
gfx_measure_text(Font_Atlas *atlas, String8 text)
{
	Text_Metrics metrics = {0};

	f32 line_height = font_atlas_height(atlas);
	Glyph_Info probe;
	font_atlas_get_glyph(atlas, 'A', &probe);
	f32 mono_advance = probe.xadvance;

	f32 x = 0.0f;
	f32 y = 0.0f;
	f32 max_x = 0.0f;

	usize consumed = 0;
	for (usize i = 0; i < text.len; i += consumed)
	{
		rune c = 0;
		UTF8_Error err = utf8_decode(text, i, &c, &consumed);

		if (err)
		{
			log_error("utf8 decoding error code : %d", err);
			break;
		}

		if (c == '\n')
		{
			max_x = Max(max_x, x);
			x = 0.0f;
			y += line_height;
			continue;
		}
		if (c == ' ')
		{
			x += mono_advance;
			continue;
		}
		if (c == '\t')
		{
			x += mono_advance * 4.0f;
			continue;
		}

		x += mono_advance;
	}

	max_x = Max(max_x, x);

	metrics.size.x = max_x;
	metrics.size.y = y + line_height;
	metrics.min = (vec2_f32){0.0f, 0.0f};
	metrics.max = (vec2_f32){metrics.size.x, metrics.size.y};

	return metrics;
}

internal vec2_f32
gfx_align_text_position(Font_Atlas *atlas, String8 text, vec2_f32 pos,
												Text_Align h_align, Text_Align v_align)
{
	Text_Metrics metrics = gfx_measure_text(atlas, text);
	vec2_f32 aligned_pos = pos;

	switch (h_align)
	{
	case Text_Align_Left:
		break;
	case Text_Align_Center:
		aligned_pos.x -= metrics.size.x * 0.5f;
		break;
	case Text_Align_Right:
		aligned_pos.x -= metrics.size.x;
		break;
	}

	switch (v_align)
	{
	case Text_Align_Top:
		break;
	case Text_Align_Middle:
		aligned_pos.y -= metrics.size.y * 0.5f;
		break;
	case Text_Align_Bottom:
		aligned_pos.y -= metrics.size.y;
		break;
	}

	aligned_pos.x = floorf(aligned_pos.x);
	aligned_pos.y = floorf(aligned_pos.y);

	return aligned_pos;
}

internal void
gfx_push_text(String8 text, vec2_f32 pos, color8_t color)
{
	Font_Atlas *atlas = &_gfx_state->font_atlas;

	usize i = 0;

	f32 x = pos.x;
	f32 y = pos.y;

	f32 atlas_w = (f32)atlas->atlas_width;
	f32 atlas_h = (f32)atlas->atlas_height;
	f32 line_height = font_atlas_height(atlas);

	Glyph_Info probe;
	font_atlas_get_glyph(atlas, 'A', &probe);
	f32 mono_advance = probe.xadvance;

	usize consumed = 0;
	for (usize i = 0; i < text.len; i += consumed)
	{
		rune c = 0;
		UTF8_Error err = utf8_decode(text, i, &c, &consumed);

		if (err)
		{
			log_error("utf8 decoding error code : %d", err);
			break;
		}
		if (c == '\n')
		{
			x = pos.x;
			y += line_height;
			continue;
		}
		if (c == ' ')
		{
			x += mono_advance;
			continue;
		}
		if (c == '\t')
		{
			x += mono_advance * 4.0f;
			continue;
		}

		Glyph_Info info;
		if (!font_atlas_get_glyph(atlas, c, &info))
		{
			font_atlas_get_glyph(atlas, '?', &info);
		}

		f32 glyph_x = x + info.xoff;
		f32 glyph_y = y + atlas->font.ascent + info.yoff;

		f32 gw = (f32)(info.x1 - info.x0);
		f32 gh = (f32)(info.y1 - info.y0);

		if (gw > 0 && gh > 0)
		{
			gfx_push_rect((Rect_Params){
				.position = {floorf(glyph_x), floorf(glyph_y)},
				.size = {gw, gh},
				.color = color,
				.uv = {
					(f32)info.x0 / atlas_w,
					(f32)info.y0 / atlas_h,
					(f32)info.x1 / atlas_w,
					(f32)info.y1 / atlas_h},
				.tex_id = atlas->tex_id,
			});
		}

		x += mono_advance;
	}
}

internal void
gfx_push_text_aligned(String8 text, vec2_f32 pos, color8_t color, Text_Align h_align, Text_Align v_align)
{
	Font_Atlas *atlas = &_gfx_state->font_atlas;

	vec2_f32 aligned_pos = gfx_align_text_position(atlas, text, pos, h_align, v_align);
	gfx_push_text(text, aligned_pos, color);
}

internal u32
gfx_texture_upload(Texture_Data data, Texture_Kind type)
{
	Assert(_gfx_state && type >= 0 && type < TextureKind_Count);
	Assert(data.data && data.channels <= 4 && data.channels >= 1 && data.width > 0 && data.height > 0);

	u32 result = 0;
	Texture *handle = NULL;

	if (_gfx_state->texture_freelist != 0)
	{
		result = _gfx_state->texture_freelist;
		handle = &_gfx_state->texture_slots[result];
		_gfx_state->texture_freelist = handle->next;
	}
	else
	{
		result = _gfx_state->texture_count;
		if (result >= MAX_TEXTURES)
		{
			return WHITE_TEXTURE;
		}
		_gfx_state->texture_count += 1;
	}

	handle = &_gfx_state->texture_slots[result];
	*handle = (Texture){0};

	glGenTextures(1, &handle->gl_id);

	i32 internal_format;
	u32 format;
	switch (data.channels)
	{
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
		handle->next = _gfx_state->texture_freelist;
		_gfx_state->texture_freelist = result;
		return WHITE_TEXTURE;
	}

	glActiveTexture(GL_TEXTURE0 + result);
	glBindTexture(GL_TEXTURE_2D, handle->gl_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (type == TextureKind_GreyScale && data.channels == 1)
	{
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
			data.data);

	glGenerateMipmap(GL_TEXTURE_2D);

	return result;
}

internal bool
gfx_texture_update(u32 id, i32 w, i32 h, i32 channels, u8 *data)
{
	Assert(_gfx_state && data && w > 0 && h > 0 && channels > 0);

	if (id >= _gfx_state->texture_count)
	{
		return false;
	}

	Texture *handle = &_gfx_state->texture_slots[id];
	if (handle->gl_id == 0)
	{
		return false;
	}

	u32 format;
	switch (channels)
	{
	case 1:
		format = GL_RED;
		break;
	case 2:
		format = GL_RG;
		break;
	case 3:
		format = GL_RGB;
		break;
	case 4:
		format = GL_RGBA;
		break;
	default:
		return false;
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
			data);
	glGenerateMipmap(GL_TEXTURE_2D);
	return true;
}

internal bool
gfx_texture_unload(u32 id)
{
	Assert(_gfx_state);

	if (id >= _gfx_state->texture_count)
	{
		return false;
	}

	Texture *handle = &_gfx_state->texture_slots[id];
	if (handle->gl_id == 0)
	{
		return false;
	}

	glDeleteTextures(1, &handle->gl_id);
	handle->gl_id = 0;
	handle->next = _gfx_state->texture_freelist;
	_gfx_state->texture_freelist = id;

	return true;
}

internal void
gfx_begin_clip(i32 x, i32 y, i32 w, i32 h)
{
	Assert(_gfx_state);

	if (_gfx_state->render_buffer.vtx_count > 0) {
		gfx_flush();
		gfx_prep();
	}

	Scissor_Rect new_rect = {x, y, w, h};

	if (_gfx_state->scissor_stack_len > 0) {
		Scissor_Rect *parent = &_gfx_state->scissor_stack[_gfx_state->scissor_stack_len - 1];

		i32 x1 = Max(new_rect.x, parent->x);
		i32 y1 = Max(new_rect.y, parent->y);
		i32 x2 = Min(new_rect.x + new_rect.w, parent->x + parent->w);
		i32 y2 = Min(new_rect.y + new_rect.h, parent->y + parent->h);

		new_rect.x = x1;
		new_rect.y = y1;
		new_rect.w = Max(0, x2 - x1);
		new_rect.h = Max(0, y2 - y1);
	}

	Assert(_gfx_state->scissor_stack_len < MAX_SCISSOR_DEPTH);
	_gfx_state->scissor_stack[_gfx_state->scissor_stack_len] = new_rect;

	_gfx_state->scissor_stack_len += 1;

	if (!_gfx_state->scissor_enabled) {
		glEnable(GL_SCISSOR_TEST);
		_gfx_state->scissor_enabled = true;
	}

	i32 gl_y = _gfx_state->viewport.h - (new_rect.y + new_rect.h);
	glScissor(new_rect.x, gl_y, new_rect.w, new_rect.h);
}

internal void
gfx_end_clip(void)
{
	Assert(_gfx_state);
	Assert(_gfx_state->scissor_stack_len > 0);

	if (_gfx_state->render_buffer.vtx_count > 0) {
		gfx_flush();
		gfx_prep();
	}

	_gfx_state->scissor_stack_len -= 1;

	if (_gfx_state->scissor_stack_len > 0) {
		Scissor_Rect *current = &_gfx_state->scissor_stack[_gfx_state->scissor_stack_len - 1];
		i32 gl_y = _gfx_state->viewport.h - (current->y + current->h);
		glScissor(current->x, gl_y, current->w, current->h);
	} else {
		glDisable(GL_SCISSOR_TEST);
		_gfx_state->scissor_enabled = false;
	}
}
