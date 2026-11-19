#ifndef RENDER_FREETYPE_H
#define RENDER_FREETYPE_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include FT_SYNTHESIS_H

typedef struct R_FT_Font R_FT_Font;
struct R_FT_Font
{
  String8    data;
  U64        hash;
  S64        ref_count;
  FT_Face    face;
  R_FT_Font *hash_prev;
  R_FT_Font *hash_next;
  Arena     *data_arena; // only holds a copy of the `data`
  R_FT_Font *free_next;
};

typedef struct R_FT_Bucket R_FT_Bucket;
struct R_FT_Bucket
{
  R_FT_Font *first;
  R_FT_Font *last;
};

typedef struct R_FT_State R_FT_State;
struct R_FT_State
{
  Arena       *arena;
  FT_Library   ft_lib;
  R_FT_Bucket *fonts;
  R_FT_Font   *free_fonts;
};

global R_FT_State *r_ft_state = 0;

#endif // R_FREETYPE_H 
