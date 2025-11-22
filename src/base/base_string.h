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

internal String8  str8(u8 *raw, usize len);
internal String8  str8_slice(String8 string, usize start, usize end_exclusive);
internal String8  str8_cstr_slice(const char* cstring, isize start, isize end_exclusive);
internal rune_itr str8_decode_utf8(u8 *raw, usize len);
internal usize    str8_codepoint_count(String8 str);
internal String8  str8_to_ctring(Arena *arena, String8 string);
internal String8  str8_copy_cstr(Arena *arena, const char *cstring);
internal String8  str8_encode_rune(rune codepoint, u8 backing_mem[4]);
internal bool     str8_equal(String8 s1, String8 s2);

internal bool     rune_is_space(rune codepoint);

internal usize    utf8_codepoint_size(u8 lead);

force_inline bool
utf8_trail(u8 byte)
{
	return (byte & 0xC0) == 0x80;
}

force_inline usize
utf8_size_from_first_byte(u8 b) {
	if ((b & 0x80) == 0) return 1;
	if ((b & 0xE0) == 0xC0) return 2;
	if ((b & 0xF0) == 0xE0) return 3;
	if ((b & 0xF8) == 0xF0) return 4;
	return 1;
}

force_inline rune
utf8_decode(u8 *p)
{
	u8 b0 = p[0];
	if ((b0 & 0x80) == 0) {
		return (rune)b0;
	}
	else if ((b0 & 0xE0) == 0xC0) {
		return (rune)(((b0 & 0x1F) << 6) |
		       (p[1] & 0x3F));
	}
	else if ((b0 & 0xF0) == 0xE0) {
		return (rune)(((b0 & 0x0F) << 12) |
		       ((p[1] & 0x3F) << 6) |
		       (p[2] & 0x3F));
	}
	else if ((b0 & 0xF8) == 0xF0) {
		return (rune)(((b0 & 0x07) << 18) |
		       ((p[1] & 0x3F) << 12) |
		       ((p[2] & 0x3F) << 6) |
		       (p[3] & 0x3F));
	}
	else {
		return 0xFFFD;
	}
}

#define STR_FMT "%.*s"

#define str8_foreach(str, itr_name, offset_name) \
for (usize offset_name = 0; offset_name < (str).len;) \
	for (rune_itr itr_name = str8_decode_utf8((str).raw + offset_name, (str).len - offset_name); \
		offset_name < (str).len && itr_name.consumed != 0; \
		offset_name += itr_name.consumed, itr_name.consumed = 0)

#endif
