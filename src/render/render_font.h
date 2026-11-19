#ifndef RENDER_FONT_H
#define RENDER_FONT_H

/////////////////////////////
// @section: basic font typess

typedef struct R_Font R_Font;
struct R_Font
{
  U64 u64[1];
};

typedef struct R_FontMetrics R_FontMetrics;
struct R_FontMetrics
{
  F32 ascent;
  F32 descent;
  F32 line_height;
};

typedef U64 R_FontAllocFlags;
enum
{
  R_FontAllocFlag_NoCopy = (1u << 0),
};

typedef struct R_FontRaster R_FontRaster;
struct R_FontRaster
{
  U8 *data;
  Vec2S32 dim;
  Vec2S16 offset;
  F32 advance;
};

typedef struct R_Glyph R_Glyph;
struct R_Glyph
{
  Rng2S16 rect;
  Vec2S16 offset;
  F32 advance;
};

typedef struct R_GlyphNode R_GlyphNode;
struct R_GlyphNode
{
  R_GlyphNode *hash_next;
  R_GlyphNode *hash_prev;
  U128 key;
  R_Glyph glyph;
  R_GlyphNode *free_next;
};

typedef struct R_GlyphHashBucket R_GlyphHashBucket;
struct R_GlyphHashBucket
{
  R_GlyphNode *first;
  R_GlyphNode *last;
};

typedef struct R_FontState R_FontState;
struct R_FontState
{
  Arena *arena;
  R_Handle texture;
  R_GlyphHashBucket slots[1024];
  R_GlyphNode *free_glyphs;
  Vec2S32 cursor;
  S16 row_height;
};

global R_FontState *r_fnt_state = 0;

/////////////////////////////
// @section: basic handle functions

function B32 r_fnt_zero(R_Font f);
function B32 r_fnt_matches(R_Font a, R_Font b);

/////////////////////////////
// @section: font caching

function void r_fnt_init(void);
function R_Glyph r_fnt_get_glyph(R_Font font, U32 size, B32 bold, U32 index);
function void r_fnt_frame(void);

/////////////////////////////
// @section: pushing draw commands

function Vec2F32 r_push_text_simple( R_DrawList *list, 
                                     R_Font font,
                                     String8 text, 
                                     Vec2F32 pos, 
                                     U32 size, 
                                     S32 tab_width, 
                                     B32 bold,
                                     Vec4F32 fg );

#define r_measure_text_simple(font, text, size, tab) \
  r_push_text_simple(0, (font), (text), V2(0), (size), (tab), false, V4(0))

/////////////////////////////
// @section: implemented by font provider

function R_Font r_fnt_alloc(String8 data, R_FontAllocFlags flags);
function void   r_fnt_free(R_Font font);

function R_FontMetrics r_fnt_metrics(R_Font font, U32 size);
function U32           r_fnt_glyph_index(R_Font font, U32 codepoint);
function R_FontRaster  r_fnt_raster(R_Font font, U32 size, U32 index, B32 bold, Arena *arena);

function void   r_fnt_provider_init(void);

#endif // RENDER_FONT_H
