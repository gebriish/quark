#ifndef GFX_CORE_H
#define GFX_CORE_H

/////////////////////////////////////////////////////////////////////
//	
//	~geb: This is the graphics layer. It is the OpenGL Renderer combined
//	with window handling and all window related handling. For now
//	I dont plan on adding any other rendering API backends other
//	than OpenGL, so this might seem pretty hardcoded. But it is, for
//	now atleast, the unified inferface through which all rendering
//	and gfx related calls happen through.
//
/////////////////////////////////////////////////////////////////////


#include "base.h"

#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL

#undef internal // base.h define has name collisions
#include "thirdparty/rgfw/rgfw.h"
#define internal static

////////////////
// ~geb: types


typedef u32 color8_t;
#define color_r(x) (((x) >> 24) & 0xFF)
#define color_g(x) (((x) >> 16) & 0xFF)
#define color_b(x) (((x) >> 8)  & 0xFF)
#define color_a(x) ((x) & 0xFF)

typedef RGFW_window* Window_Handle;
typedef u32 Shader_Program;

typedef u32 Pixel_Format;
enum {
	Pixel_R8 = 0,
	Pixel_RG8,
	Pixel_RGB8,
	Pixel_RGBA8,
};

typedef u32 Texture_Kind;
enum {
	TextureKind_Normal = 0,
	TextureKind_GreyScale,
};


typedef struct {
	u32 width;
	u32 height;
	Pixel_Format pixel_fmt;
	u8 *data;
} Image;

#define WHITE_TEXTURE 1

typedef u32 Render_Flags;
enum {
	Render_Wireframe = Bit(0),
};

typedef struct {
	vec2 position;
	color8_t color;
	u16_vec2 texcoords;
	u16_vec2 circle_mask_coord;
} Vertex_2D;

#define VTX_SIZE sizeof(Vertex_2D)

typedef struct {
	vec2 from, to;
} Rect;

#define CIRC_CENTER (Rect) {{0.5,0.5}, {0.5,0.5}}
#define UV_FULL (Rect) {{0,0}, {1,1}}


////////////////
// ~geb: utility fns

internal bool rect_vs_rect(Rect r1, Rect r2);

////////////////
// ~geb: main renderer

#define MAX_TRIANGLES     1024
#define MAX_VERTEX_COUNT  MAX_TRIANGLES * 3

typedef u32 Quad_Shader_Uniforms;
enum {
	Uniform_Proj,
	Uniform_Texture,
	Uniform_Count
};

typedef struct {
	Vertex_2D *vertices;
	u16       *indices;
	u32 vertex_count;
	u32 index_count;
} Render_Batch;

typedef struct {
	Allocator allocator;
	Allocator temp_allocator;

	Window_Handle  window;
	i32 mouse_x, mouse_y;


	Shader_Program quad_shader;
	i32            quad_uniforms[Uniform_Count];

	Render_Batch render_batch;

	// bind state
	u32 active_texture;

	// geoemetru
	u32 batch_vao;
	u32 batch_vbo;
	u32 batch_ebo;

	ivec2         resolution;
	f64           frame_delta;
	OS_Time_Stamp last_frame_time;
	Render_Flags  render_flags;
} GFX_Context;

enum {
	Pressed_Backspace = Bit(0),
	Pressed_Enter = Bit(1),
	Pressed_Delete = Bit(2),

	Pressed_Move_Left = Bit(3),
	Pressed_Move_Right = Bit(4),
	Pressed_Move_Up = Bit(5),
	Pressed_Move_Down = Bit(6),
};

typedef struct {
	String8 text;
	u32 special_key_presses;
} Frame_Input;

///////////////////////
// ~geb: Graphics State

internal GFX_Context  gfx_new(String8 title_cstring, i32 w, i32 h, Allocator allocator, Allocator temp_allocator);
internal GFX_Context *gfx_set_context(GFX_Context *ctx);
internal bool         gfx_window_open();
internal void         gfx_mouse_position(f32 *x, f32 *y);
internal f64          gfx_delta_time();
internal Rect         gfx_get_clip_rect();

///////////////////////
// ~geb: Rendering API

internal Shader_Program gfx_compile_program(String8 content);
internal void           gfx_delete_program(Shader_Program program);

internal u32  gfx_texture_upload(Image data, Texture_Kind type);
internal void gfx_texture_unload(u32 tex_id);
internal void gfx_texture_sub_data(u32 tex_id, i32 x, i32 y, Image img);

internal Frame_Input gfx_frame_begin(color8_t col);
internal void gfx_frame_end();

internal void gfx_push_rect(vec2 pos, vec2 size, color8_t color, u32 texture, Rect tex_coords, Rect circle_coords);

#define gfx_shader_load_uniforms(program, locations, enum_count, enum_to_string)                 \
do {                                                                                             \
	for (int i=0; i<cast(int)(enum_count); ++i) {                                                \
		(locations)[i] = glGetUniformLocation((program), (const char *)(enum_to_string)[i].str); \
	}                                                                                            \
} while(0)

#endif

