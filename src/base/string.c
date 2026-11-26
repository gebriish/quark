
function b32
char_is_space(u8 c)
{
  return (c >= '\t' && c <= '\r') || c == ' ';
}

read_only global u8 utf8_len_table[32] = 
{
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 0xxxxxxx
  0,0,0,0,0,0,0,0,                   // 10xxxxxx (invalid lead)
  2,2,2,2,                           // 110xxxxx
  3,3,                               // 1110xxxx
  4,                                 // 11110xxx
  0,                                 // 11111xxx (invalid)
};

read_only u8 first_mask[5] = { 0, 0x7F, 0x1F, 0x0F, 0x07 };

function u32
utf8_decode(String8 text, s64 index, s32 *out_incr)
{
  u32 codepoint = 0xFFFD;
  s32 incr = 1;

  if(0 <= index && index < (s64)text.len)
  {
    u8 *p   = text.raw + index;
    s32 len = utf8_len_table[p[0] >> 3];

    if(len && index + len <= text.len)
    {
      u32 c = p[0] & first_mask[len];
      s32 i = 1;
      for(; i < len && (p[i] & 0xC0) == 0x80; i += 1)
      {
        c = (c << 6) | (p[i] & 0x3F);
      }
      if(i == len)
      {
        local_persist u32 min_cp[5] = { 0, 0, 0x80, 0x800, 0x10000 };
        if(c >= min_cp[len] && c <= 0x10FFFF && (c < 0xD800 || c > 0xDFFF))
        {
          codepoint = c;
          incr      = len;
        }
      }
    }
  }

  if(out_incr) { *out_incr = incr; }
  return codepoint;
}

function s64
utf8_encode(u8 *str, u32 codepoint)
{
  s64 inc = 0;
  if(codepoint <= 0x7F)
  {
    str[0] = (u8)codepoint;
    inc = 1;
  }
  else if(codepoint <= 0x7FF)
  {
    str[0] = 0xC0 | (u8)(codepoint >> 6);
    str[1] = 0x80 | (u8)(codepoint & 0x3F);
    inc = 2;
  }
  else if(codepoint <= 0xFFFF)
  {
    str[0] = 0xE0 | (u8)(codepoint >> 12);
    str[1] = 0x80 | (u8)((codepoint >> 6) & 0x3F);
    str[2] = 0x80 | (u8)(codepoint & 0x3F);
    inc = 3;
  }
  else if(codepoint <= 0x10FFFF)
  {
    str[0] = 0xF0 | (u8)(codepoint >> 18);
    str[1] = 0x80 | (u8)((codepoint >> 12) & 0x3F);
    str[2] = 0x80 | (u8)((codepoint >> 6) & 0x3F);
    str[3] = 0x80 | (u8)(codepoint & 0x3F);
    inc = 4;
  }
  else
  {
    str[0] = '?';
    inc = 1;
  }
  return inc;
}

// @section: string8

function String8 
str8(u8 *raw, s64 len)
{
  String8 result = 
  {
    .raw = raw,
    .len = len
  };
  return result;
}

function String8 
str8_cstring(u8 *cstring)
{
  String8 result = {0};
  result.raw = cstring;
  result.len = (s64) strlen((char *) cstring);
  return result;
}

function String8 
str8_slice(String8 src, s64 begin, s64 end)
{
  begin = Clamp(0, begin, src.len);
  end = Clamp(begin, end, src.len);

  String8 result = {0};
  result.raw = src.raw + begin;
  result.len = end - begin;
  return result;
}


function const u8 *
str8_push_cstring(Arena *arena, String8 src)
{
  u8 *str = arena_push(arena, u8, src.len + (s64) 1);
  MemMove(str, src.raw, src.len);
  str[src.len] = 0;
  return str;
}


function b32 
str8_matches(String8 a, String8 b)
{
  return (a.len == b.len) && (memcmp(a.raw, b.raw, (size_t) a.len) == 0);
}

function String8 
str8_prefix(String8 src, s64 len)
{
  return str8_slice(src, 0, len);
}

function String8 
str8_skip(String8 src, s64 amnt)
{
  return str8_slice(src, amnt, src.len);
}

function String8 
str8_postfix(String8 src, s64 len)
{
  return str8_slice(src, src.len - len, src.len);
}

function String8 
str8_strip(String8 src)
{
  s64 begin = 0;
  s64 end = src.len;

  while (begin < end && char_is_space(src.raw[begin])) begin += 1;
  while (end > begin && char_is_space(src.raw[end-1])) end -= 1;

  return str8_slice(src, begin, end);
}

function String8 
str8_push_copy(Arena *arena, String8 src)
{
  String8 result = {0};
  result.raw = arena_push(arena, u8, src.len);
  result.len = src.len;
  MemMove(result.raw, src.raw, src.len);
  return result;
}

function u64
str8_hash(String8 string)
{
  u64 hash = 14695981039346656037ull;

  u8 *ptr = string.raw;
  u8 *end = ptr + string.len;

  while(ptr < end)
  {
    hash ^= *ptr++;
    hash *= 1099511628211ull;
  }

  return hash;
}

function String8
str8_format(Arena *arena, const char *fmt_string, ...)
{
	va_list args;

	va_start(args, fmt_string);
	int len = vsnprintf(0, 0, fmt_string, args);
	va_end(args);

	if (len < 0) return S("");

	u8 *buf = arena_push(arena, u8, len + 1);

	va_start(args, fmt_string);
	vsnprintf((char *)buf, (size_t) len + (size_t) 1, fmt_string, args);
	va_end(args);

	return str8(buf, len);
}

function s64
str8_find_first(String8 string, u32 cp)
{
	s64 i = 0;
	while (i < string.len)
	{
		s32 width = 0;
		u32 decoded = utf8_decode(string, i, &width);

		if (decoded == cp)
		{
			break;
		}

		i += 1;
	}

	return Min(i, string.len);
}

function s64
str8_find_last(String8 string, u32 cp)
{
	s64 i = string.len;
	while (i > 0)
	{
		s64 pos = i - 1;
		while (pos > 0 && utf8_tail(string.raw[pos]))
		{
			pos -= 1;
		}

		s32 width = 0;
		u32 decoded = utf8_decode(string, pos, &width);
		if (decoded == cp)
		{
			return pos;
		}

		i = pos;
	}
	return -1;
}

function b32
str8_starts_with(String8 src, String8 pre)
{
  b32 result = false;
  if (src.len >= pre.len)
  {
    result = (memcmp(src.raw, pre.raw, (size_t) pre.len) == 0);
  }
  return result;
}

// @section: string8 array

function String8_Array 
str8_array(String8 *raw, s64 capacity)
{
  String8_Array result = {0};
  result.raw = raw;
  result.capacity = capacity;
  result.len = 0;
  return result;
}


// @section: string8 list

function void 
str8_list_push(Arena *arena, String8_List *list, String8 str)
{
  String8_Node *node = arena_push(arena, String8_Node, 1);
  node->data = str;
  SLLQueuePush(list->first, list->last, node);
  list->len += 1;
}

function String8 
str8_list_join(Arena *arena, String8_List *list, u8 sep)
{
  String8 result = {0};

  if (list->len > 0)
  {
    b32 sep_exist = (sep != 0);
    s64 buf_size = (list->len - 1) * sep_exist;

    for each_node(node, String8_Node, list->first) 
    {
      buf_size += node->data.len;
    }
    result.raw = arena_push_non_zero(arena, u8, buf_size);

    s64 stack_top = 0;
    for each_node(node, String8_Node, list->first) 
    {
      String8 string = node->data;
      MemMove(result.raw + stack_top, string.raw, string.len);
      stack_top += string.len;

      if (stack_top < buf_size)
      {
        result.raw[stack_top] = sep;
        stack_top += (s64) sep_exist;
      }
    }

    result.len = buf_size;
  }

  return result;
}

