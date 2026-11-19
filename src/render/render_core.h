#ifndef RENDER_CORE_H
#define RENDER_CORE_H

const S64 QUAD_BATCH_SIZE = 2048;

typedef U32 R_DrawKind;
enum
{
  R_DrawKind_Quads,
  R_DrawKind_Blur,
  R_DrawKind_Text,
  R_DrawKind_ClipPush,
  R_DrawKind_ClipPop,
  R_DrawKind_COUNT,
};

typedef struct R_Handle R_Handle;
struct R_Handle
{
  U64 u64[1];
};

typedef struct R_Quad R_Quad;
struct R_Quad
{
  Rng2F32 dst;
  Rng2F32 src;
  Vec4F32 col;
  Vec4F32 radii;
  F32 thickness;
  F32 edge_blur;
};

typedef struct R_Blur R_Blur;
struct R_Blur
{
  Rng2F32 rect;
  Vec4F32 tint;
  Vec4F32 radii;
  F32 blur;
};

typedef struct R_TextQuad R_TextQuad;
struct R_TextQuad
{
  Rng2F32 dst;
  Rng2F32 src;
  Vec4F32 fg;
};

typedef struct R_QuadBatch R_QuadBatch;
struct R_QuadBatch
{
  R_Quad  *raw;
  S64      len;
  R_Handle tex;
};

typedef struct R_TextBatch R_TextBatch;
struct R_TextBatch
{
  R_TextQuad *raw;
  S64         len;
  R_Handle    tex;
};

typedef struct R_Draw R_Draw;
struct R_Draw
{
  R_DrawKind kind;

  union
  {
    R_QuadBatch quad_batch;
    R_TextBatch text_batch;
    R_Blur      blur;
    Rng2S32     rect;
  };

  R_Draw *next;
};

typedef struct R_DrawList R_DrawList;
struct R_DrawList
{
  R_Draw *first;
  R_Draw *last;
  S64        len;
};

typedef U32 R_TexFormat;
enum
{
  R_TexFormat_R8,
  R_TexFormat_RG8,
  R_TexFormat_RGB8,
  R_TexFormat_RGBA8,
};

typedef struct R_Image R_Image;
struct R_Image
{
  R_TexFormat fmt;
  Vec2S32 dim;
  U8 *data;
};

/////////////////////////////
// @section: handles

function R_Handle r_empty_handle(void);
function B32 r_handle_zero(R_Handle h);
function B32 r_handle_matches(R_Handle a, R_Handle b);

/////////////////////////////
// @section: state functions

function void   r_init(void);
function Arena *r_frame_arena(void);

/////////////////////////////
// @section: frame marker hooks

function void r_window_begin_frame(GFX_Window win);
function void r_window_end_frame(GFX_Window win);
function void r_window_set_vsync(GFX_Window win, B32 vsync);

function F32  r_frame_begin(void);
function void r_frame_end(void);
function void r_window_submit_frame(R_DrawList list);

/////////////////////////////
// @section: texture management

function R_Handle r_tex_alloc(R_Image image);
function void     r_tex_free(R_Handle h);
function void     r_tex_update(R_Handle h, Rng2S32 range, U8 *data);
function Vec2S32  r_tex_size(R_Handle h);
function R_Image  r_blank_image(R_TexFormat fmt, Vec2S32 dim);

/////////////////////////////
// @section: pushing render commands

function void r_push_quad_(R_DrawList *list, R_Handle tex, R_Quad *quad);
function void r_push_blur_(R_DrawList *list, R_Blur *blur);
function void r_push_text_quad_(R_DrawList *list, R_Handle tex, R_TextQuad *q);

function void r_push_clip(R_DrawList *list, Rng2F32 rect); // just making it f32 range for ease
function void r_pop_clip(R_DrawList *list);

/////////////////////////////
// @section: macros

#define r_push_quad(l, t, ...) r_push_quad_((l), (t), &(R_Quad) { __VA_ARGS__ })
#define r_push_blur(l, ...) r_push_blur_((l), &(R_Blur) { __VA_ARGS__ })
#define r_push_text_quad(l, t, ...) r_push_text_quad_((l), (t), &(R_TextQuad) { __VA_ARGS__ })

#define WindowFrameScope(w)  DeferLoop(r_window_begin_frame(w), r_window_end_frame(w))
#define Radius4(x) (Vec4F32) { x,x,x,x }
#define Color8(x) V4(          \
  ((x >> 24) & 0xff) / 255.0f, \
  ((x >> 16) & 0xff) / 255.0f, \
  ((x >>  8) & 0xff) / 255.0f, \
  ((x >>  0) & 0xff) / 255.0f  \
)

#define Color_White V4(1.f,1.f,1.f,1.f)

#endif // RENDER_CORE_H
