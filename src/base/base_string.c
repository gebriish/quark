#include "base_string.h"

global const u8 utf8_class[256] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x00-0x0F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x10-0x1F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x20-0x2F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x30-0x3F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x40-0x4F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x50-0x5F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x60-0x6F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x70-0x7F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x80-0x8F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x90-0x9F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xA0-0xAF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xB0-0xBF
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // 0xC0-0xCF
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // 0xD0-0xDF
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, // 0xE0-0xEF
	4,4,4,4,4,4,4,4,5,5,5,5,6,6,0,0  // 0xF0-0xFF
};

internal String8 
str8(u8 *raw, usize len)
{
	String8 str = {
		raw,
		len
	};
	return str;
}

internal String8 
str8_slice(String8 string, usize start, usize end_exclusive)
{
	AssertAlways(start >= 0 && end_exclusive <= string.len && end_exclusive > start);
	return str8(string.raw + start, Min(string.len, end_exclusive - start));
}

internal String8
str8_cstr_slice(const char *cstring, isize start, isize end_exclusive)
{
	AssertAlways(cstring != NULL);
	usize len = MemStrlen(cstring);

	if (end_exclusive < 0) {
		end_exclusive = (isize) len;
	}

	AssertAlways(start >= 0 && end_exclusive <= (isize)len && end_exclusive > start);

	return str8((u8 *)(cstring + start), (usize)(end_exclusive - start));
}

internal rune_itr 
str8_decode_utf8(u8 *raw, usize len) {
	rune_itr result = {0};

	if (len == 0) {
		result.codepoint = UNICODE_REPLACEMENT;
		result.consumed = 0;
		return result;
	}

	u8 first = raw[0];
	u8 byte_count = utf8_class[first];

	if (byte_count == 0 || byte_count > len) {
		result.codepoint = UNICODE_REPLACEMENT;
		result.consumed = 1;
		return result;
	}

	result.consumed = byte_count;

	switch (byte_count) {
		case 1: {
			result.codepoint = first;
		} break;

		case 2: {
			u8 b1 = raw[0];
			u8 b2 = raw[1];

			if ((b2 & 0xC0) != 0x80) {
				result.codepoint = UNICODE_REPLACEMENT;
				result.consumed = 1;
				break;
			}

			rune cp = (rune)(((b1 & 0x1F) << 6) | (b2 & 0x3F));

			if (cp < 0x80) {
				result.codepoint = UNICODE_REPLACEMENT;
				result.consumed = 1;
			} else {
				result.codepoint = cp;
			}
		} break;

		case 3: {
			u8 b1 = raw[0];
			u8 b2 = raw[1];
			u8 b3 = raw[2];

			if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
				result.codepoint = UNICODE_REPLACEMENT;
				result.consumed = 1;
				break;
			}

			rune cp = (rune)(((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F));

			if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
				result.codepoint = UNICODE_REPLACEMENT;
				result.consumed = 1;
			} else {
				result.codepoint = cp;
			}
		} break;

		case 4: {
			u8 b1 = raw[0];
			u8 b2 = raw[1];
			u8 b3 = raw[2];
			u8 b4 = raw[3];

			if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80 || (b4 & 0xC0) != 0x80) {
				result.codepoint = UNICODE_REPLACEMENT;
				result.consumed = 1;
				break;
			}

			rune cp = (rune)(((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12) | 
					((b3 & 0x3F) << 6) | (b4 & 0x3F));

			if (cp < 0x10000 || cp > 0x10FFFF) {
				result.codepoint = UNICODE_REPLACEMENT;
				result.consumed = 1;
			} else {
				result.codepoint = cp;
			}
		} break;

		default: {
			result.codepoint = UNICODE_REPLACEMENT;
			result.consumed = 1;
		} break;
	}

	return result;
}

internal usize 
str8_codepoint_count(String8 str) {
	usize count = 0;
	usize offset = 0;

	while (offset < str.len) {
		rune_itr itr = str8_decode_utf8(str.raw + offset, str.len - offset);
		if (itr.consumed == 0) break;

		offset += itr.consumed;
		count++;
	}

	return count;
}

internal String8
str8_to_ctring(Arena *arena, String8 string)
{
	String8 result = {0};
	result.raw = arena_push_array(arena, u8, string.len + 1);
	MemMove(result.raw, string.raw, string.len);
	result.raw[string.len] = '\0';
	result.len = string.len;
	return result;
}

internal String8
str8_copy_cstr(Arena *arena, const char *cstring)
{
	usize string_len = MemStrlen(cstring);
	String8 result;
	result.raw = arena_push(arena, string_len + 1, AlignOf(u8), false);
	result.len = string_len;
	MemMove(result.raw, cstring, string_len);
	result.raw[string_len] = 0;
	return result;
}

internal String8
str8_encode_rune(rune codepoint, u8 backing_mem[4])
{
	String8 result = {0};

	if (codepoint <= 0x7F) {
		backing_mem[0] = (u8)codepoint;
		result.len = 1;
	}
	else if (codepoint <= 0x7FF) {
		backing_mem[0] = 0xC0 | (u8)(codepoint >> 6);        // first 5 bits
		backing_mem[1] = 0x80 | (u8)(codepoint & 0x3F);      // last 6 bits
		result.len = 2;
	}
	else if (codepoint <= 0xFFFF) {
		backing_mem[0] = 0xE0 | (u8)(codepoint >> 12);                  // first 4 bits
		backing_mem[1] = 0x80 | (u8)((codepoint >> 6) & 0x3F);          // middle 6 bits
		backing_mem[2] = 0x80 | (u8)(codepoint & 0x3F);                 // last 6 bits
		result.len = 3;
	}
	else if (codepoint <= 0x10FFFF) {
		backing_mem[0] = 0xF0 | (u8)(codepoint >> 18);                  // first 3 bits
		backing_mem[1] = 0x80 | (u8)((codepoint >> 12) & 0x3F);         // next 6 bits
		backing_mem[2] = 0x80 | (u8)((codepoint >> 6) & 0x3F);          // next 6 bits
		backing_mem[3] = 0x80 | (u8)(codepoint & 0x3F);                 // last 6 bits
		result.len = 4;
	}
	else {
		backing_mem[0] = 0xEF;
		backing_mem[1] = 0xBF;
		backing_mem[2] = 0xBD;
		result.len = 3;
	}

	result.raw = backing_mem;
	return result;
}

internal bool
rune_is_space(rune cp)
{
	return cp == ' ' || cp == '\t' || cp == '\r';
}

internal usize
utf8_codepoint_size(u8 lead)
{
    if ((lead & 0x80) == 0x00) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1;
}
