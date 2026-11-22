#ifndef RENDER_FONT_H
#define RENDER_FONT_H

/////////////////////////////
// @section: basic font typess

typedef struct Fnt_Handle Fnt_Handle;
struct Fnt_Handle
{
  u64 v[1];
};

typedef struct Fnt_Metrics Fnt_Metrics;
struct Fnt_Metrics
{
  f32 ascent;
  f32 descent;
  f32 line_height;
};

typedef u64 Font_Alloc_Flags;
enum
{
  Font_Alloc_Flag_NoCopy = (1u << 0),
};

typedef struct Font_Raster Font_Raster;
struct Font_Raster
{
  u8 *data;
  Vec2S32 dim;
  Vec2S16 offset;
  f32 advance;
};

typedef struct Glyph_Rect Glyph_Rect;
struct Glyph_Rect
{
  Rng2S16 rect;
  Vec2S16 offset;
  f32 advance;
};

typedef struct Glyph_Node Glyph_Node;
struct Glyph_Node
{
  Glyph_Node *next;
  Glyph_Node *prev;
  u128 key;
  Glyph_Rect glyph;
};

typedef struct Glyph_Hash_Bucket Glyph_Hash_Bucket;
struct Glyph_Hash_Bucket
{
  Glyph_Node *first;
  Glyph_Node *last;
};

typedef struct Fnt_State Fnt_State;
struct Fnt_State
{
  Arena *arena;
  Rndr_Handle texture;
  Glyph_Hash_Bucket slots[1024];
  Glyph_Node *free_glyphs;
  Vec2S32 cursor;
  s16 row_height;
};

global Fnt_State *fnt_state = 0;

/////////////////////////////
// @section: basic handle functions

function b32 fnt_handle_zero(Fnt_Handle f);
function b32 fnt_handle_matches(Fnt_Handle a, Fnt_Handle b);

/////////////////////////////
// @section: font caching

function void fnt_init(void);
function Glyph_Rect fnt_get_glyph(Fnt_Handle font, u32 size, b32 bold, u32 index);
function void fnt_frame(void);

/////////////////////////////
// @section: pushing draw commands

function Vec2F32 push_text_simple(Draw_List *list, 
                                  Fnt_Handle font,
                                  String8 text, 
                                  Vec2F32 pos, 
                                  u32 size, 
                                  s32 tab_width, 
                                  b32 bold,
                                  Vec4F32 fg);

#define measure_text_simple(font, text, size, tab) \
  push_text_simple(0, (font), (text), v2f32(0), (size), (tab), false, v4f32(0))

/////////////////////////////
// @section: implemented by font provider

function void fnt_provider_init(void);

function Fnt_Handle fnt_alloc(String8 data, Font_Alloc_Flags flags);
function void   fnt_free(Fnt_Handle font);

function Fnt_Metrics fnt_metrics(Fnt_Handle font, u32 size);
function u32 get_glyph_index(Fnt_Handle font, u32 codepoint);
function Font_Raster rasterize_glyph(Fnt_Handle font, u32 size, u32 index, b32 bold, Arena *arena);
function f32 get_kerning(Fnt_Handle font, u32 size, u32 left, u32 right);

#endif // RENDER_FONT_H
