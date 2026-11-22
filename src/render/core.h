#ifndef RENDER_CORE_H
#define RENDER_CORE_H

const s64 QUAD_BATCH_SIZE = 2048;

typedef u32 Draw_Kind;
enum
{
  Draw_Kind_Quads,
  Draw_Kind_Blur,
  Draw_Kind_Text,
  Draw_Kind_ClipPush,
  Draw_Kind_ClipPop,
  Draw_Kind_COUNT,
};

typedef struct Rndr_Handle Rndr_Handle;
struct Rndr_Handle
{
  u64 v[1];
};

typedef struct Quad_Inst Quad_Inst;
struct Quad_Inst
{
  Rng2F32 dst;
  Rng2F32 src;
  Vec4F32 col;
  Vec4F32 radii;
  f32 thickness;
  f32 edge_blur;
};

typedef struct Quad_Blur Quad_Blur;
struct Quad_Blur
{
  Rng2F32 rect;
  Vec4F32 tint;
  Vec4F32 radii;
  f32 blur;
};

typedef struct Quad_Text Quad_Text;
struct Quad_Text
{
  Rng2F32 dst;
  Rng2F32 src;
  Vec4F32 fg;
};

typedef struct Quad_Batch Quad_Batch;
struct Quad_Batch
{
  Quad_Inst  *raw;
  s64        len;
  Rndr_Handle   tex;
};

typedef struct Text_Batch Text_Batch;
struct Text_Batch
{
  Quad_Text *raw;
  s64       len;
  Rndr_Handle  tex;
};

typedef struct Draw_Call Draw_Call;
struct Draw_Call
{
  Draw_Kind kind;

  union
  {
    Quad_Batch quad_batch;
    Text_Batch text_batch;
    Quad_Blur   blur;
    Rng2S32     rect;
  };

  Draw_Call *next;
};

typedef struct Draw_List Draw_List;
struct Draw_List
{
  Draw_Call *first;
  Draw_Call *last;
  s64   len;
};

typedef u32 Pixel_Format;
enum
{
  Pixel_Format_R8,
  Pixel_Format_RG8,
  Pixel_Format_RGB8,
  Pixel_Format_RGBA8,
};

typedef struct Image Image;
struct Image
{
  Pixel_Format fmt;
  Vec2S32 dim;
  u8 *data;
};

typedef struct Clip_Rect Clip_Rect;
struct Clip_Rect
{
  Rng2S32 rect;
  Clip_Rect *next;
};

/////////////////////////////
// @section: handles

function Rndr_Handle r_empty_handle(void);
function b32 r_handle_zero(Rndr_Handle h);
function b32 r_handle_matches(Rndr_Handle a, Rndr_Handle b);

/////////////////////////////
// @section: state functions

function void   rndr_init(void);
function Arena *frame_arena(void);

/////////////////////////////
// @section: frame marker hooks

function void window_frame_begin(Win_Handle win);
function void window_frame_end(Win_Handle win);
function void render_window_vsync(Win_Handle win, b32 vsync);

function f32  frame_begin(void);
function void frame_end(void);
function void submit_draw_list(Draw_List list);

/////////////////////////////
// @section: texture management

function Rndr_Handle texture_alloc(Image image);
function void        texture_free(Rndr_Handle h);
function void        texture_update(Rndr_Handle h, Rng2S32 range, u8 *data);
function Vec2S32     texture_size(Rndr_Handle h);

/////////////////////////////
// @section: pushing render commands

function void push_quad_(Draw_List *list, Rndr_Handle tex, Quad_Inst *quad);
function void push_blur_(Draw_List *list, Quad_Blur *blur);
function void push_text_quad_(Draw_List *list, Rndr_Handle tex, Quad_Text *q);

// just making it f32 range for ease
function void push_clip(Draw_List *list, Rng2F32 rect); 
function void pop_clip(Draw_List *list);

/////////////////////////////
// @section: macros

#define push_quad(l, t, ...) push_quad_((l), (t), &(Quad_Inst) { __VA_ARGS__ })
#define push_blur(l, ...) push_blur_((l), &(Quad_Blur) { __VA_ARGS__ })
#define push_text_quad(l, t, ...) push_text_quad_((l), (t), &(Quad_Text) { __VA_ARGS__ })

#define WindowFrameScope(w)  DeferLoop(window_frame_begin(w), window_frame_end(w))
#define Radius4(x) (Vec4F32) { x,x,x,x }
#define Color8(x) v4f32(       \
  ((x >> 24) & 0xff) / 255.0f, \
  ((x >> 16) & 0xff) / 255.0f, \
  ((x >>  8) & 0xff) / 255.0f, \
  ((x >>  0) & 0xff) / 255.0f  \
)

#define Color_White v4f32(1.f,1.f,1.f,1.f)

#endif // RENDER_CORE_H
