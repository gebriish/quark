#ifndef BASE_STRING_H
#define BASE_STRING_H

#include "base_core.h"

#define UNICODE_REPLACEMENT 0xFFFD

typedef u32 rune;

typedef struct {
  rune codepoint;
  u32  consumed;
} rune_itr;


typedef struct String8 String8; // UTF8 Encoded String
struct String8 {
  u8 *raw;
  usize len;
};

#define str8_lit(x) (String8){.raw = (u8 *)(x), .len = sizeof(x) - 1}
#define str8_fmt(x) ((i32)x.len), (x.raw)

internal_lnk String8  str8(u8 *raw, usize len);
internal_lnk String8  str8_slice(String8 string, usize i, usize len);
internal_lnk rune_itr str8_decode_utf8(u8 *raw, usize len);
internal_lnk usize    str8_codepoint_count(String8 str);
internal_lnk String8  str8_cstr_copy(Arena *arena, String8 string);

#define STR_FMT "%.*s"

#define str8_foreach(str, itr_name, offset_name) \
  for (usize offset_name = 0; offset_name < (str).len;) \
    for (rune_itr itr_name = str8_decode_utf8((str).raw + offset_name, (str).len - offset_name); \
         offset_name < (str).len && itr_name.consumed != 0; \
         offset_name += itr_name.consumed, itr_name.consumed = 0)

#define FONT_CHARSET \
  "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" /* ASCII */ \
  "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞß" \
  "àáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿ" /* Latin-1 Supplement */ \
  "ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ" \
  "αβγδεζηθικλμνξοπρστυφχψω" /* Greek */ \
  "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" \
  "абвгдежзийклмнопрстуфхцчшщъыьэюя" /* Cyrillic */ \

#endif
