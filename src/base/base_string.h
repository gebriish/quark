#ifndef BASE_STRING_H
#define BASE_STRING_H

////////////////////////////////////
// @section: types

typedef struct String8 String8;
struct String8
{
  u8 *raw;
  s64 len;
};

typedef struct String8_Array String8_Array;
struct String8_Array
{
  String8 *raw;
  s64      capacity;
  s64      len;
};

typedef struct String8_Node String8_Node;
struct String8_Node
{
  String8 data;
  String8_Node *next;
};

typedef struct String8_List String8_List;
struct String8_List
{
  String8_Node *first;
  String8_Node *last;
  s64 len;
};

////////////////////////////////////
// @section: utility procs and macros

#define S(x) (String8) { (u8 *) x, sizeof(x) - 1 }
#define str8_varg(x) (int) ((x).len), (char *) ((x).raw)
#define F_STR8 "%.*s"

function b32 char_is_space(u8 c);
function u32 utf8_decode(String8 text, s64 index, s32 *out_incr);
function s64 utf8_encode(u8 *str, u32 codepoint);

#define utf8_tail(b) (((b) & 0xC0) == 0x80)

////////////////////////////////////
// @section: string8

function String8 str8(u8 *raw, s64 len);
function String8 str8_cstring(u8 *cstring);
function const u8 *str8_push_cstring(Arena *arena, String8 src);
function b32 str8_matches(String8 a, String8 b);
function String8 str8_push_copy(Arena *arena, String8 src);
function u64 str8_hash(String8 string);
function String8 str8_format(Arena *arena, const char *fmt_string, ...); /* returns a null terminated string */

////////////////////////////////////
// @section: string8 slicing

function String8 str8_slice(String8 src, s64 begin, s64 end);
function String8 str8_prefix(String8 src, s64 len);
function String8 str8_skip(String8 src, s64 amnt);
function String8 str8_postfix(String8 src, s64 len);
function String8 str8_strip(String8 src);

function s64 str8_find_first(String8, u32 c);
function s64 str8_find_last(String8, u32 c);

////////////////////////////////////
// @section: string8 array

function String8_Array str8_array(String8 *raw, s64 capacity);

////////////////////////////////////
// @section: string8 list

function void str8_list_push(Arena *arena, String8_List *list, String8 str);
function String8 str8_list_join(Arena *arena, String8_List *list, u8 sep);

#endif // BASE_STRING_H
