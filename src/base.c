#include "base.h"

/////////////////////////////////////////////////////////////////////////
//                            OS Layer                                 //
/////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdarg.h>

#if OS_LINUX
#include "linux/base_linux.c"
#else
#error "OS Implementations missing"
#endif

internal OS_Time_Duration
os_time_diff(OS_Time_Stamp start, OS_Time_Stamp end)
{
	OS_Time_Duration result = {0};

	u64 freq    = os_time_frequency();
	u64 elapsed = end - start;

	result.seconds      = (f64)elapsed / (f64)freq;
	result.milliseconds = result.seconds * 1000.0;
	result.microseconds = result.seconds * 1000000.0;

	return result;
}

internal String8
os_data_from_path(String8 path, Allocator alloc)
{
	String8 result = {0};

	OS_Handle file = os_file_open(OS_AccessFlag_Read, path);
	if (file < 0) return result;

	OS_FileProps props = os_properties_from_file(file);
	if (props.size == 0)
	{
		os_file_close(file);
		return result;
	}

	u8 *mem = alloc_array(alloc, u8, props.size, NULL);
	if (!mem)
	{
		os_file_close(file);
		return result;
	}

	usize read = os_file_read(file, 0, props.size, mem);
	if (read == props.size)
	{
		result.str = mem;
		result.len = props.size;
	}

	os_file_close(file);
	return result;
}

internal bool
os_write_to_path(String8 path, String8 data)
{
	OS_Handle file = os_file_open(OS_AccessFlag_Write, path);
	if (file < 0) return false;

	usize written = os_file_write(file, 0, data.len, data.str);
	os_file_close(file);

	return (written == data.len);
}


/////////////////////////////////////////////////////////////////////////
//                        ALLOCATORS                                   //
/////////////////////////////////////////////////////////////////////////

// ~geb: general perpose (gp) allocator

#include <stdlib.h>

force_inline void *
heap_realloc(void *original_ptr, usize new_size)
{
	return realloc(original_ptr, new_size);
}

force_inline void *
heap_alloc(usize size)
{
	return malloc(size);
}

force_inline void
heap_free(void *ptr)
{
	free(ptr);
}

internal void *
_gp_aligned_alloc(usize size, usize alignment, void *old_ptr, usize old_size, bool mem_zero, Alloc_Error *err)
{
	if (err)
		*err = Alloc_Err_None;
	if (size == 0)
		return NULL;

	alignment = Max(alignment, AlignOf(void *));

	usize space = size + alignment - 1 + sizeof(void *);
	u8 *raw = heap_alloc(space);
	if (!raw)
	{
		if (err)
			*err = Alloc_Err_OOM;
		return NULL;
	}

	u8 *aligned = (u8 *)AlignPow2((usize)(raw + sizeof(void *)), alignment);
	((void **)aligned)[-1] = raw;

	if (mem_zero)
	{
		MemZero(aligned, size);
	}

	return aligned;
}

internal void
_gp_aligned_free(void *p)
{
	if (!p)
		return;
	heap_free(((void **)p)[-1]);
}

internal void *
_gp_aligned_resize(void *p, size_t old_size, size_t new_size, size_t new_alignment, bool zero_memory, Alloc_Error *err)
{
	if (err)
		*err = Alloc_Err_None;

	if (!p)
	{
		return _gp_aligned_alloc(new_size, new_alignment, NULL, 0, zero_memory, err);
	}

	if (new_size == 0)
	{
		_gp_aligned_free(p);
		return NULL;
	}

	new_alignment = Max(new_alignment, AlignOf(void *));

	void *old_raw = ((void **)p)[-1];

	usize space = new_size + new_alignment - 1 + sizeof(void *);
	u8 *new_raw = heap_realloc(old_raw, space);
	if (!new_raw)
	{
		if (err)
			*err = Alloc_Err_OOM;
		return NULL;
	}

	u8 *new_aligned =
		(u8 *)AlignPow2((usize)(new_raw + sizeof(void *)), new_alignment);
	((void **)new_aligned)[-1] = new_raw;

	if (new_aligned != p)
	{
		MemMove(new_aligned, p, Min(old_size, new_size));
	}

	if (zero_memory && new_size > old_size)
	{
		MemZero(new_aligned + old_size, new_size - old_size);
	}

	return new_aligned;
}

internal Allocator_Proc(gp_allocator_proc)
{
	switch (type)
	{
	case Allocation_Alloc_Non_Zero:
	case Allocation_Alloc:
		return _gp_aligned_alloc(size, alignment, NULL, 0, type == Allocation_Alloc, err);

	case Allocation_Free:
		_gp_aligned_free(old_memory);
		break;

	case Allocation_FreeAll:
		*err = Alloc_Err_Mode_Not_Implemented;
		return NULL;

	case Allocation_Resize_Non_Zero:
	case Allocation_Resize:
		return _gp_aligned_resize(old_memory, old_size, size, alignment, type == Allocation_Resize, err);
	}

	if (err)
		*err = Alloc_Err_None;
	return NULL;
}

internal Allocator
heap_allocator(void)
{
	return (Allocator){
		.proc = gp_allocator_proc,
		.data = NULL};
}

// ~geb: arena allocator
#define COMMIT_BLOCK_SIZE Kb(64)

internal Arena *
_arena_make(usize reserve_size)
{
	if (reserve_size > USIZE_MAX - sizeof(Arena))
	{
		return NULL;
	}

	usize total_size = sizeof(Arena) + reserve_size;

	void *base = os_reserve(total_size);
	if (!base)
	{
		return NULL;
	}

	usize header_commit_size = AlignPow2(sizeof(Arena), COMMIT_BLOCK_SIZE);

	if (os_commit(base, header_commit_size))
	{
		os_release(base, total_size);

		return NULL;
	}

	Arena *arena = cast(Arena *) base;

	arena->pos = 0;
	arena->base = cast(u8 *) base + sizeof(Arena);
	arena->reserved = reserve_size;
	arena->committed = header_commit_size > sizeof(Arena) ? header_commit_size - sizeof(Arena) : 0;
	return arena;
}

internal void
_arena_free_all(Arena *arena)
{
	Assert(arena);
	arena->pos = 0;
}

internal void *
_arena_alloc_aligned(Arena *arena, usize size, usize alignment, bool zero, Alloc_Error *err)
{
	if (err)
		*err = Alloc_Err_None;

	usize aligned_pos = AlignPow2(arena->pos, alignment);
	usize new_pos = aligned_pos + size;

	if (new_pos > arena->reserved)
	{
		*err = Alloc_Err_OOM;
		return NULL;
	}

	if (new_pos > arena->committed)
	{
		usize needed = new_pos - arena->committed;
		usize commit_size = AlignPow2(needed, COMMIT_BLOCK_SIZE);

		if (arena->committed + commit_size > arena->reserved)
		{
			commit_size = arena->reserved - arena->committed;
		}

		if (commit_size == 0)
		{
			*err = Alloc_Err_OOM;
			return NULL;
		}

		void *commit_ptr = arena->base + arena->committed;
		if (os_commit(commit_ptr, commit_size) != 0)
		{
			*err = Alloc_Err_OOM;
			return NULL;
		}

		arena->committed += commit_size;
	}

	arena->pos = new_pos;
	if (zero) {
		MemZero(arena->base + aligned_pos, size);
	}
	return arena->base + aligned_pos;
}

internal void *
_arena_realloc_aligned(Arena *arena, void *ptr, usize old_size, usize new_size, usize alignment, bool zero, Alloc_Error *err)
{
	*err = Alloc_Err_None;

	if (!arena)
	{
		*err = Alloc_Err_Invalid_Argument;
		return NULL;
	}

	if (!ptr)
	{
		*err = Alloc_Err_Invalid_Pointer;
		return NULL;
	}

	if (new_size == 0)
	{
		*err = Alloc_Err_Invalid_Argument;
		return NULL;
	}

	u8 *expected_end = arena->base + arena->pos;
	u8 *actual_end = (u8 *)ptr + old_size;

	bool is_last_alloc = (expected_end == actual_end);

	if (is_last_alloc)
	{
		usize offset = (u8 *)ptr - (u8 *)arena->base;
		usize new_pos = offset + new_size;

		if (new_pos > arena->reserved)
		{
			*err = Alloc_Err_OOM;
			return NULL;
		}

		if (new_pos > arena->committed)
		{
			usize needed = new_pos - arena->committed;
			usize commit_size = AlignPow2(needed, COMMIT_BLOCK_SIZE);

			if (arena->committed + commit_size > arena->reserved)
			{
				commit_size = arena->reserved - arena->committed;
			}

			if (commit_size == 0)
			{
				*err = Alloc_Err_OOM;
				return NULL;
			}

			void *commit_ptr = arena->base + arena->committed;
			if (os_commit(commit_ptr, commit_size) != 0)
			{
				*err = Alloc_Err_OOM;
				return NULL;
			}

			arena->committed += commit_size;
		}

		arena->pos = new_pos;
		return ptr;
	}

	void *new_ptr = _arena_alloc_aligned(
		arena,
		new_size,
		alignment,
		zero,
		err);

	if (!new_ptr)
	{
		return NULL;
	}

	usize copy_size = old_size < new_size ? old_size : new_size;
	MemMove(new_ptr, ptr, copy_size);

	return new_ptr;
}

internal Allocator_Proc(arena_allocator_proc)
{
	Arena *arena = (Arena *)allocator_data;

	switch (type)
	{
	case Allocation_Alloc_Non_Zero:
	case Allocation_Alloc:
		return _arena_alloc_aligned(arena, size, alignment, type == Allocation_Alloc, err);

	case Allocation_Resize_Non_Zero:
	case Allocation_Resize:
		return _arena_realloc_aligned(
			arena, old_memory, old_size, size,
			alignment, type == Allocation_Resize, err);

	case Allocation_Free:
		*err = Alloc_Err_Mode_Not_Implemented;
		break;

	case Allocation_FreeAll:
		_arena_free_all(arena);
		break;
	}

	return NULL;
}

internal Allocator
arena_allocator(usize reserve)
{
	Arena *arena = _arena_make(reserve);

	return (Allocator){
		.proc = arena_allocator_proc,
		.data = arena};
}

internal Arena_Scope
arena_scope_begin(Arena *arena)
{
	Assert(arena);

	Arena_Scope scope = {
		.arena = arena,
		.pos = arena->pos
	};
	return scope;
}

internal void
arena_scope_end(Arena_Scope scope)
{
	Assert(scope.arena && scope.pos <= scope.arena->pos);

	Arena *arena = scope.arena;
	arena->pos = scope.pos;
}

/////////////////////////////////////////////////////////////////////////
//                            STRINGS                                  //
/////////////////////////////////////////////////////////////////////////

force_inline bool utf8_is_cont(u8 b)
{
	return (b & 0xC0) == RUNE_SELF;
}

internal rune
utf8_decode(u8 *ptr, UTF8_Error *err)
{
	Assert(ptr);
	if (err)
		*err = UTF8_Err_None;

	u8 *p = ptr;
	u8 b0 = p[0];

	u8 len = UTF8_LEN_TABLE[b0];
	if (len == 0)
	{
		if (err)
			*err = UTF8_Err_InvalidLead;
		return 0;
	}

	rune r = 0;

	switch (len)
	{
	case 1:
		r = b0;
		break;

	case 2:
		if (!utf8_is_cont(p[1]))
			goto invalid_cont;
		r = ((b0 & 0x1F) << 6) |
			(p[1] & 0x3F);
		if (r < RUNE_SELF)
			goto overlong;
		break;

	case 3:
		if (!utf8_is_cont(p[1]) || !utf8_is_cont(p[2]))
			goto invalid_cont;
		r = ((b0 & 0x0F) << 12) |
			((p[1] & 0x3F) << 6) |
			(p[2] & 0x3F);
		if (r < 0x800)
			goto overlong;
		if (r >= 0xD800 && r <= 0xDFFF)
			goto surrogate;
		break;

	case 4:
		if (!utf8_is_cont(p[1]) || !utf8_is_cont(p[2]) || !utf8_is_cont(p[3]))
			goto invalid_cont;
		r = ((b0 & 0x07) << 18) |
			((p[1] & 0x3F) << 12) |
			((p[2] & 0x3F) << 6) |
			(p[3] & 0x3F);
		if (r < 0x10000)
			goto overlong;
		if (r > 0x10FFFF)
			goto out_of_range;
		break;
	}

	return r;

invalid_cont:
	if (err)
		*err = UTF8_Err_InvalidContinuation;
	return 0;

overlong:
	if (err)
		*err = UTF8_Err_Overlong;
	return 0;

surrogate:
	if (err)
		*err = UTF8_Err_Surrogate;
	return 0;

out_of_range:
	if (err)
		*err = UTF8_Err_OutOfRange;
	return 0;
}

internal bool
is_letter(rune r)
{
	if (r < RUNE_SELF)
	{
		if ((r >= 'A' && r <= 'Z') ||
			(r >= 'a' && r <= 'z') ||
			r == '_') {
			return true;
		}
	}

	return false; // TODO: unicode
}

internal bool
is_digit(rune r)
{
	if ('0' <= r && r <= '9') { return true; }
	return false; // TODO: unicode
}

internal bool
is_space(rune r)
{
	return (r == cast(rune) ' ' ||
			r == cast(rune) '\t' ||
			r == cast(rune) '\n' ||
			r == cast(rune) '\r');
}

internal bool
is_pair_begin(rune r)
{
	switch (r) {
		case '(':
		case '{':
		case '[':
		case '\'':
		case '\"':
			return true;
	}
	return false;
}

internal String8
get_pair_end(rune r)
{
	switch (r) {
		case '(': return S(")");
		case '{': return S("}");
		case '[': return S("]");
		case '\'': return S("\'");
		case '\"': return S("\"");
		default: return S("");
	}
}

internal String8
str8(u8 *ptr, usize len)
{
	String8 string = {
		.str = ptr,
		.len = len,
	};
	return string;
}

internal String8
str8_make(const char *cstring, Allocator allocator)
{
	usize len = MemStrlen(cstring);
	String8 string = {
		.len = len,
	};

	Alloc_Error err = 0;
	string.str = alloc_array(allocator, u8, len, &err);
	MemMove(string.str, cstring, len);

	if (err)
	{
		return S("");
	}

	return string;
}

internal Alloc_Error
str8_delete(Allocator alloc, String8 *str)
{
	Allocator a = alloc;
	if (!a.proc)
	{
		return Alloc_Err_Mode_Not_Implemented;
	}

	Alloc_Error err = 0;
	mem_free(a, str->str, &err);

	if (err)
	{
		return err;
	}

	return Alloc_Err_None;
}


internal String8
str8_concat(String8 s1, String8 s2, Allocator alloc)
{
	if (!s1.len && !s2.len) return S("");

	usize combined_len = s1.len + s2.len;

	Alloc_Error err = 0;
	u8 *buffer = alloc_array_nz(alloc, u8, combined_len, &err);
	if (err) return S("");

	MemMove(buffer, s1.str, s1.len);
	MemMove(buffer + s1.len, s2.str, s2.len);

	String8 result;
	result.str = buffer;
	result.len = combined_len;
	return result;
}

internal isize
find_left(String8 str, rune c)
{
	for(Str_Iterator itr = {0}; str8_iter(str, &itr);)
	{
		if (itr.codepoint == c) {
			return cast(isize)(itr.ptr - str.str);
		}
	}
	return -1;
}

internal isize
find_right(String8 str, rune target)
{
    u8 *p = str.str + str.len;

    while (p > str.str) {
        p -= 1;

        while (p > str.str && (*p & 0xC0) == RUNE_SELF) {
            p -= 1;
        }

        Str_Iterator itr = { .ptr = p };
        str8_iter(str, &itr);

        if (itr.codepoint == target) {
            return (isize)(p - str.str);
        }
    }
    return -1;
}

internal String8_List
str8_make_list(const char **cstrings, usize count, Allocator allocator)
{
	String8_List list = dynamic_array(allocator, String8, count);
	if (!list.data) { return (String8_List){0}; }

	for (usize i = 0; i < count; ++i)
	{
		dyn_arr_append(&list, String8, str8_make(cstrings[i], allocator));
	}

	return list;
}

internal Alloc_Error
str8_delete_list(String8_List *list)
{
	Allocator a = list->alloc;
	if (!a.proc)
	{
		return Alloc_Err_Mode_Not_Implemented;
	}

	String8 *string_array = cast(String8 *) list->data;
	for (usize i = 0; i < list->len; ++i)
	{
		Alloc_Error err = str8_delete(list->alloc, &string_array[i]);
		if (err)
		{
			return err;
		}
	}

	dynamic_array_delete(list);

	return Alloc_Err_None;
}

internal String8
str8_list_index(String8_List *list, usize i)
{
	Assert(list);
	Assert(i < list->len);

	return (cast(String8 *)(list->data))[i];
}

internal String8
str8_slice(String8 string, usize begin, usize end_exclusive)
{
	Assert(begin <= end_exclusive);
	Assert(end_exclusive <= string.len);

	String8 result = {0};
	result.str = string.str + begin;
	result.len = end_exclusive - begin;

	return result;
}

internal bool
str8_equal(String8 first, String8 second)
{
	if (first.len != second.len)
	{
		return false;
	}

	return MemCompare(first.str, second.str, first.len) == 0;
}

internal String8
str8_copy(String8 string, Allocator alloc)
{
	u8 *mem = alloc_array(alloc, u8, string.len, NULL);
	MemMove(mem, string.str, string.len);
	String8 str = {
		.str = mem,
		.len = string.len};
	return str;
}

internal String8
str8_copy_cstring(String8 string, Allocator alloc)
{
	u8 *mem = alloc_array(alloc, u8, string.len + 1, NULL);
	MemMove(mem, string.str, string.len);
	mem[string.len] = '\0';
	String8 str = {
		.str = mem,
		.len = string.len + 1};
	return str;
}

internal String8
str8_file_extension(String8 path)
{
	if (path.len == 0) return S("");

	for (usize i = path.len; i-- > 0; ) {
		if (path.str[i] == '.') {
			return str8_slice(path, i + 1, path.len);
		}
	}

	return S("");
}

internal String8
str8_file_name(String8 path)
{
	if (path.len == 0) return S("");

	usize start = 0;

	for (usize i = path.len; i-- > 0;) {
		rune c = path.str[i];
		if (c == '/' || c == '\\') {
			start = i + 1;
			break;
		}
	}

	String8 name = str8_slice(path, start, path.len);

	if (name.len == 0) return name;

	for (usize i = name.len; i-- > 0;) {
		if (name.str[i] == '.') {
			if (i == 0) break;
			return str8_slice(name, 0, i);
		}
	}
	return name;
}

internal String8
str8_tprintf(Allocator alloc, const char *fmt, ...)
{
    String8 result = {0};

    if (!fmt) {
        return result;
    }

    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);

    int required = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (required < 0) {
        va_end(args);
        return result; // formatting error
    }

    size_t size = (size_t)required;

    uint8_t *buffer = alloc_array_nz(alloc, u8, size + 1, NULL);
    if (!buffer) {
        va_end(args);
        return result; // allocation failure
    }

    int written = vsnprintf((char *)buffer, size + 1, fmt, args);
    va_end(args);

    if (written < 0) {
		mem_free(alloc, buffer, NULL);
        return result;
    }

    result.str = buffer;
    result.len = size;
    return result;
}


internal bool
str8_iter(String8 string, Str_Iterator *it)
{
	Assert(it);

	u8 *end = string.str + string.len;

	if (!it->ptr)
	{
		if (string.len == 0)
			return false;
		it->ptr = string.str;
	}
	else
	{
		it->ptr += it->width;
	}

	if (it->ptr >= end)
		return false;

	u8 lead = *it->ptr;
	u32 width = UTF8_LEN_TABLE[lead];
	Assert(width > 0 && width <= 4);
	Assert(it->ptr + width <= end);

	UTF8_Error err = UTF8_Err_None;
	it->codepoint = utf8_decode(it->ptr, &err);
	Assert(err == UTF8_Err_None);

	it->width = width;
	return true;
}

/////////////////////////////////////////////////////////////////////////
//                            LOGGER                                   //
/////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>

#define ANSI_RESET   "\x1b[0m"
#define ANSI_GRAY    "\x1b[90m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_RED     "\x1b[31m"

internal void log_base(const char* color,
                     const char* level,
                     const char* fmt,
                     va_list args)
{
    printf("%s[ %s ] %s", color, level, ANSI_RESET);
    vprintf(fmt, args);
    printf("\n");
}

internal void log_debug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_base(ANSI_GRAY, "DEBUG", fmt, args);
    va_end(args);
}

internal void log_info(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_base(ANSI_GREEN, "INFO ", fmt, args);
    va_end(args);
}

internal void log_warn(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_base(ANSI_YELLOW, "WARN ", fmt, args);
    va_end(args);
}

internal void log_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_base(ANSI_RED, "ERROR", fmt, args);
    va_end(args);
}


/////////////////////////////////////////////////////////////////////////
//                      DYNAMIC ARRAY                                  //
/////////////////////////////////////////////////////////////////////////



internal void
dynamic_array_delete(Dynamic_Array *arr)
{
	if (arr->data) {
		Alloc_Error err = 0;
		mem_free(arr->alloc, arr->data, &err);
	}

	arr->data = NULL;
	arr->len = 0;
	arr->capacity = 0;
}


internal bool
dynamic_array_reserve(Dynamic_Array *arr, usize elem_size, usize elem_align, usize min_capacity)
{
	if (min_capacity <= arr->capacity)
		return true;

	usize new_capacity = arr->capacity ? arr->capacity : 32;
	while (new_capacity < min_capacity) {
		new_capacity <<= 1;
	}

	Alloc_Error err = 0;
	void *new_data = mem_resize_aligned(
		arr->alloc,
		arr->data,
		arr->capacity * elem_size,
		new_capacity * elem_size,
		elem_align,
		false,
		&err
	);

	if (err)
		return false;

	arr->data     = new_data;
	arr->capacity = new_capacity;
	return true;
}

internal void
dynamic_array_clear(Dynamic_Array *arr)
{
	arr->len = 0;
}
