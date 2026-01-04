#include "base_string.h"

internal String8
str8(u8 *mem, usize len)
{
	String8 str = {
			mem,
			len};
	return str;
}

internal String8
str8_slice(String8 string, usize start, usize end_exclusive)
{
	AssertAlways(start >= 0 && end_exclusive <= string.len && end_exclusive > start);
	return str8(string.str + start, Min(string.len, end_exclusive - start));
}

internal String8
str8_concat(String8 left, String8 right, Arena *arena)
{
	Assert(arena != 0);

	usize total_len = left.len + right.len;
	Assert(total_len >= left.len);

	u8 *str_data = arena_push_array(arena, u8, total_len);

	MemMove(str_data, left.str, left.len);
	MemMove(str_data + left.len, right.str, right.len);

	return str8(str_data, total_len);
}

internal String8
str8_sprintf(Arena *arena, const char *fmt, ...)
{
	va_list args, args_copy;
	va_start(args, fmt);

	va_copy(args_copy, args);
	int size = vsnprintf(NULL, 0, fmt, args_copy);
	va_end(args_copy);

	if (size < 0)
	{
		va_end(args);
		return S("");
	}

	u8 *buf = arena_push_array(arena, u8, size + 1);
	if (!buf)
	{
		va_end(args);
		return S("");
	}

	vsnprintf((char *)buf, size + 1, fmt, args);
	va_end(args);

	return str8(buf, size);
}

internal bool
str8_equal(String8 s1, String8 s2)
{
	if (s1.len != s2.len)
		return false;
	return MemCompare(s1.str, s2.str, s1.len) == 0;
}

internal usize
str8_count(String8 string)
{
	usize consumed = 0, count = 0;
	for (usize i=0; i<string.len; i+=consumed, ++count)
	{
		rune c = 0;
		UTF8_Error err = utf8_decode(string, i, &c, &consumed);

		if (err != UTF8_Err_None) { return count; }
	}

	return count;
}


internal string8_list
str8_list_from_cstring_array(Arena *arena, usize count, char **cstrings)
{
	Assert(arena);
	Assert(count != 0 && cstrings);

	string8_list arr = string8_list_make(arena, count);

	for (usize i = 0; i < count; i++)
	{
		usize len = MemStrlen(cstrings[i]);
		String8 str = str8((u8 *)cstrings[i], len);

		string8_list_push(&arr, str);
	}

	return arr;
}

force_inline bool utf8_is_cont(u8 b)
{
	return (b & 0xC0) == 0x80;
}

internal UTF8_Error
utf8_decode(String8 s, usize idx, rune *out, usize *consumed)
{
	if (consumed)
		*consumed = 0;

	if (idx >= s.len)
	{
		return UTF8_Err_OutOfBounds;
	}

	u8 *p = s.str + idx;
	u8 b0 = p[0];

	u8 len = UTF8_SIZE[b0];
	if (len == 0)
	{
		return UTF8_Err_InvalidLead;
	}

	if (idx + len > s.len)
	{
		return UTF8_Err_OutOfBounds;
	}

	rune r = 0;

	switch (len)
	{
	case 1:
		r = b0;
		break;

	case 2:
		if (!utf8_is_cont(p[1]))
		{
			return UTF8_Err_InvalidContinuation;
		}
		r = ((b0 & 0x1F) << 6) |
				(p[1] & 0x3F);
		if (r < 0x80)
		{
			return UTF8_Err_Overlong;
		}
		break;

	case 3:
		if (!utf8_is_cont(p[1]) || !utf8_is_cont(p[2]))
		{
			return UTF8_Err_InvalidContinuation;
		}
		r = ((b0 & 0x0F) << 12) |
				((p[1] & 0x3F) << 6) |
				(p[2] & 0x3F);
		if (r < 0x800)
		{
			return UTF8_Err_Overlong;
		}
		if (r >= 0xD800 && r <= 0xDFFF)
		{
			return UTF8_Err_Surrogate;
		}
		break;

	case 4:
		if (!utf8_is_cont(p[1]) ||
				!utf8_is_cont(p[2]) ||
				!utf8_is_cont(p[3]))
		{
			return UTF8_Err_InvalidContinuation;
		}
		r = ((b0 & 0x07) << 18) |
				((p[1] & 0x3F) << 12) |
				((p[2] & 0x3F) << 6) |
				(p[3] & 0x3F);
		if (r < 0x10000)
		{
			return UTF8_Err_Overlong;
		}
		if (r > 0x10FFFF)
		{
			return UTF8_Err_OutOfRange;
		}
		break;
	}

	*out = r;
	if (consumed)
		*consumed = len;
	return UTF8_Err_None;
}
