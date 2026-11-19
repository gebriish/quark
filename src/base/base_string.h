#ifndef BASE_STRING_H
#define BASE_STRING_H

////////////////////////////////////
// @section: types

typedef struct String8 String8;
struct String8
{
  U8 *raw;
  S64 len;
};

typedef struct String8Array String8Array;
struct String8Array
{
  String8 *raw;
  S64      capacity;
  S64      len;
};

typedef struct String8Node String8Node;
struct String8Node
{
  String8 data;
  String8Node *next;
};

typedef struct String8List String8List;
struct String8List
{
  String8Node *first;
  String8Node *last;
  S64 len;
};

typedef struct String8_Builder String8_Builder;
struct String8_Builder
{
  Arena *arena;
  U8 *raw;
  S64 len;
};

////////////////////////////////////
// @section: utility procs and macros

#define S(x) (String8) { (U8 *) x, sizeof(x) - 1 }
#define str8_varg(x) (int) ((x).len), (char *) ((x).raw)
#define F_STR8 "%.*s"

function B32 char_is_space(U8 c);
function U32 utf8_decode(String8 text, S64 index, S32 *out_incr);
function S64 utf8_encode(U8 *str, U32 codepoint);

#define utf8_tail(b) (((b) & 0xC0) == 0x80)

////////////////////////////////////
// @section: string8

function String8 str8(U8 *raw, S64 len);
function String8 str8_zero(void);
function String8 str8_cstring(U8 *cstring);
function const U8 *str8_push_cstring(Arena *arena, String8 src);
function B32 str8_matches(String8 a, String8 b);
function String8 str8_push_copy(Arena *arena, String8 src);
function U64 str8_hash(String8 string);
function String8 str8_format(Arena *arena, const char *fmt_string, ...); /* returns a null terminated string */

////////////////////////////////////
// @section: string8 slicing

function String8 str8_slice(String8 src, S64 begin, S64 end);
function String8 str8_prefix(String8 src, S64 len);
function String8 str8_skip(String8 src, S64 amnt);
function String8 str8_postfix(String8 src, S64 len);
function String8 str8_strip(String8 src);

function S64 str8_find_first(String8, U32 c);
function S64 str8_find_last(String8, U32 c);

////////////////////////////////////
// @section: string8 array

function String8Array str8_array(String8 *raw, S64 capacity);

////////////////////////////////////
// @section: string8 list

function void str8_list_push(Arena *arena, String8List *list, String8 str);
function String8 str8_list_join(Arena *arena, String8List *list, U8 sep);

////////////////////////////////////
// @section: string8 builder

#endif // BASE_STRING_H
