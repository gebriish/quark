#ifndef BASE_STRING_H
#define BASE_STRING_H

#include "base_core.h"
#include "base_collections.h"
#include "base_arena.h"

#define Utf8_Invalid 0xFFFD
global const u8 UTF8_SIZE[256] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x00–0x0F
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x10–0x1F
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x20–0x2F
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x30–0x3F
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40–0x4F
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x50–0x5F
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x60–0x6F
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x70–0x7F

		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x80–0x8F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x90–0x9F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xA0–0xAF
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xB0–0xBF

		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0xC0–0xCF
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0xD0–0xDF
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // 0xE0–0xEF

		4, 4, 4, 4, 4, 4, 4, 4, // F0–F7
		0, 0, 0, 0, 0, 0, 0, 0	// F8–FF (invalid in UTF-8)
};

typedef struct String8 String8; // UTF-8 String
struct String8 {
	u8 *str;
	usize len;
};

Generate_List(String8, string8_list)

typedef u32 rune;

#define S(x) (String8) {.str = (u8 *)x, .len = (usize) sizeof(x) - 1}
#define S_FMT "%.*s"
#define str8_fmt(s) (int)s.len, (char *)s.str

force_inline String8 str8_empty() {
	return S("");
}

internal String8 str8(u8 *mem, usize len);
internal String8 str8_slice(String8 string, usize begin, usize end_exclusive);
internal String8 str8_concat(String8 left, String8 right, Arena *arena);
internal String8 str8_sprintf(Arena *arena, const char *fmt, ...);
internal bool    str8_equal(String8 s1, String8 s2);
internal usize   str8_count(String8 string);

/* ~geb: Doesnt copy string data itself, strings are a view */  
internal string8_list str8_list_from_cstring_array(Arena *arena, usize count, char **cstrings);


Enum(ASCII_Sequence, u8) {
    ASCII_NUL = 0x00, // Null
    ASCII_SOH = 0x01, // Start of Heading
    ASCII_STX = 0x02, // Start of Text
    ASCII_ETX = 0x03, // End of Text (Ctrl+C)
    ASCII_EOT = 0x04, // End of Transmission
    ASCII_ENQ = 0x05, // Enquiry
    ASCII_ACK = 0x06, // Acknowledge
    ASCII_BEL = 0x07, // Bell
    ASCII_BS  = 0x08, // Backspace
    ASCII_HT  = 0x09, // Horizontal Tab
    ASCII_LF  = 0x0A, // Line Feed
    ASCII_VT  = 0x0B, // Vertical Tab
    ASCII_FF  = 0x0C, // Form Feed
    ASCII_CR  = 0x0D, // Carriage Return
    ASCII_ESC = 0x1B, // Escape
    ASCII_DEL = 0x7F  // Delete
} ASCII_Control;

typedef u32 UTF8_Error;
enum {
	UTF8_Err_None = 0,
	UTF8_Err_OutOfBounds,
	UTF8_Err_InvalidLead,
	UTF8_Err_InvalidContinuation,
	UTF8_Err_Overlong,
	UTF8_Err_Surrogate,
	UTF8_Err_OutOfRange,
};


// UTF-8
internal UTF8_Error utf8_decode(String8 s, usize idx, rune *out, usize *consumed);
internal String8    utf8_encode(rune codepoint, u8 *buf);

force_inline bool
rune_is_space(rune c)
{
	return c == ' '  ||
	       c == '\t' ||
	       c == '\n' ||
	       c == '\r';
}


#endif
