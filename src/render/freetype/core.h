#ifndef RENDER_FREETYPE_H
#define RENDER_FREETYPE_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include FT_SYNTHESIS_H

typedef struct R_FT_Font R_FT_Font;
struct R_FT_Font
{
  R_FT_Font *hash_prev;
  R_FT_Font *hash_next;

  String8    data;
  s64        data_cap;
  b32        owns_data;

  u64        hash;
  s64        ref_count;
  FT_Face    face;
  R_FT_Font *free_next;
};

typedef struct R_FT_Bucket R_FT_Bucket;
struct R_FT_Bucket
{
  R_FT_Font *first;
  R_FT_Font *last;
};

typedef struct R_FT_FreeNode R_FT_FreeNode;
struct R_FT_FreeNode
{
  s64 size;
  R_FT_FreeNode *next;
  R_FT_FreeNode *prev;
};

typedef struct R_FT_FreeList R_FT_FreeList;
struct R_FT_FreeList
{
  R_FT_FreeNode *first;
  R_FT_FreeNode *last;
  s64 len;
};

typedef struct R_FT_State R_FT_State;
struct R_FT_State
{
  Arena        *arena;
  FT_Library    ft_lib;
  R_FT_Bucket  *fonts;
  R_FT_Font    *free_fonts;
  R_FT_FreeList data_free_list;
};

global R_FT_State *r_ft_state = 0;

function void r_ft_fnt_release_font_storage(R_FT_Font *font);


#endif // R_FREETYPE_H 
