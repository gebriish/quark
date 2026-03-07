///////////////////////////////////////////////////////////////////
// ~geb: This is the base layer that is going to be used in almost
//       every other source file. It includes some usefull macros
//       for OS/Architecture recognision, easy type shorthands,
//       compiler intrinsics and A minimal Standard library.
///////////////////////////////////////////////////////////////////

#ifndef BASE_H
#define BASE_H

////////////////////////////////
// ~geb: Compiler/OS Cracking

#if defined(__clang__)

# define COMPILER_CLANG 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__) || defined(__linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error This compiler/OS combo is not supported.
# endif

# if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ARCH_X64 1
# elif defined(i386) || defined(__i386) || defined(__i386__)
#  define ARCH_X86 1
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# elif defined(__arm__)
#  define ARCH_ARM32 1
# else
#  error Architecture not supported.
# endif


#elif defined(_MSC_VER)

# define COMPILER_MSVC 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# else
#  error This compiler/OS combo is not supported.
# endif

# if defined(_M_AMD64)
#  define ARCH_X64 1
# elif defined(_M_IX86)
#  define ARCH_X86 1
# elif defined(_M_ARM64)
#  define ARCH_ARM64 1
# elif defined(_M_ARM)
#  define ARCH_ARM32 1
# else
#  error Architecture not supported.
# endif

#elif defined(__GNUC__) || defined(__GNUG__)

# define COMPILER_GCC 1

# if defined(__gnu_linux__) || defined(__linux__)
#  define OS_LINUX 1
# else
#  error This compiler/OS combo is not supported.
# endif

# if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ARCH_X64 1
# elif defined(i386) || defined(__i386) || defined(__i386__)
#  define ARCH_X86 1
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# elif defined(__arm__)
#  define ARCH_ARM32 1
# else
#  error Architecture not supported.
# endif

#else
# error Compiler not supported.
#endif

////////////////////////////////
// ~geb: Arch Cracking

#if (ARCH_X64 || ARCH_ARM64)
# define ARCH_64BIT 1
#elif (ARCH_X86 || ARCH_ARM32)
# define ARCH_32BIT 1
#endif

////////////////////////////////
// ~geb: Zero All Undefined Options

#if !defined(ARCH_32BIT)
# define ARCH_32BIT 0
#endif
#if !defined(ARCH_64BIT)
# define ARCH_64BIT 0
#endif
#if !defined(ARCH_X64)
# define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
# define ARCH_X86 0
#endif
#if !defined(ARCH_ARM64)
# define ARCH_ARM64 0
#endif
#if !defined(ARCH_ARM32)
# define ARCH_ARM32 0
#endif
#if !defined(COMPILER_MSVC)
# define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
#endif
#if !defined(OS_WINDOWS)
# define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
# define OS_LINUX 0
#endif
#if !defined(OS_MAC)
# define OS_MAC 0
#endif

#if OS_LINUX
# define _GNU_SOURCE 1
#endif

#ifdef __cplusplus
# define IS_CPP 1
# define IS_C   0
#else
# define IS_CPP 0
# define IS_C   1
#endif

#define Paste(a, b) a ## b


#define Kb(n)   (((usize)(n)) << 10)
#define Mb(n)   (((usize)(n)) << 20)
#define Gb(n)   (((usize)(n)) << 30)

#ifndef Static_Assert
# define Static_Assert3(cond, msg) typedef char static_assertion_##msg[(!!(cond))*2-1]
# define Static_Assert2(cond, line) Static_Assert3(cond, static_assertion_at_line_##line)
# define Static_Assert1(cond, line) Static_Assert2(cond, line)
# define Static_Assert(cond)        Static_Assert1(cond, __LINE__)
#endif

#include <stdint.h>

typedef uint8_t   u8;
typedef  int8_t   i8;
typedef uint16_t u16;
typedef  int16_t i16;
typedef uint32_t u32;
typedef  int32_t i32;
typedef uint64_t u64;
typedef  int64_t i64;

#if ARCH_64BIT
typedef u64 usize;
typedef i64 isize;
#elif ARCH_32BIT
typedef u32 usize;
typedef i32 isize;
#endif

typedef float  f32;
typedef double f64;

typedef struct {u8 x,  y;} u8_vec2;
typedef struct {u16 x, y;} u16_vec2;
typedef struct {u32 x, y;} u32_vec2;
typedef struct {u64 x, y;} u64_vec2;

typedef struct {i8 x,  y;} i8_vec2;
typedef struct {i16 x, y;} i16_vec2;
typedef struct {i32 x, y;} i32_vec2;
typedef struct {i64 x, y;} i64_vec2;

typedef struct {f32 x, y;} f32_vec2;
typedef struct {f64 x, y;} f64_vec2;
#define v2(T, x, y) (Paste(T, _vec2)) {x, y}

typedef f32_vec2  vec2;
typedef i32_vec2 ivec2;

Static_Assert(sizeof(u8)  == 1);
Static_Assert(sizeof(u16) == 2);
Static_Assert(sizeof(u32) == 4);
Static_Assert(sizeof(u64) == 8);

Static_Assert(sizeof(f32) == 4);
Static_Assert(sizeof(f64) == 8);

Static_Assert(sizeof(u8)  == sizeof(i8));
Static_Assert(sizeof(u16) == sizeof(i16));
Static_Assert(sizeof(u32) == sizeof(i32));
Static_Assert(sizeof(u64) == sizeof(i64));

#ifndef cast // ~geb: This is just for grep
#define cast(Type) (Type)
#endif

typedef u32 rune; // ~geb: Unicode code point
#define Rune_Invalid (cast(rune)0xFFFD)
#define Rune_Max     (cast(rune)0x10FFFF)
#define Rune_BOM     (cast(rune)0xFEFF)


typedef u8 bool;
#define true  (0 == 0)
#define false (1 == 0)

#ifndef U8_MIN
# define U8_MIN 0u
# define U8_MAX 0xffu
# define I8_MIN (-0x7f - 1)
# define I8_MAX 0x7f

# define U16_MIN 0u
# define U16_MAX 0xffffu
# define I16_MIN (-0x7fff - 1)
# define I16_MAX 0x7fff

# define U32_MIN 0u
# define U32_MAX 0xffffffffu
# define I32_MIN (-0x7fffffff - 1)
# define I32_MAX 0x7fffffff

# define U64_MIN 0ull
# define U64_MAX 0xffffffffffffffffull
# define I64_MIN (-0x7fffffffffffffffll - 1)
# define I64_MAX 0x7fffffffffffffffll

# define F32_MIN 1.17549435e-38f
# define F32_MAX 3.40282347e+38f

# define F64_MIN 2.2250738585072014e-308
# define F64_MAX 1.7976931348623157e+308

# if ARCH_32BIT
#  define USIZE_MAX U32_MAX
#  define ISIZE_MAX I32_MAX
# elif ARCH_64BIT
#  define USIZE_MAX U64_MAX
#  define ISIZE_MAX I64_MAX
# endif
#endif

#if COMPILER_CLANG || COMPILER_GCC
# define force_inline inline __attribute__((always_inline))
#elif COMPILER_MSVC
# define force_inline __forceinline
#else
# define force_inline inline
#endif

#if COMPILER_MSVC
# define AlignOf(T) __alignof(T)
#elif COMPILER_CLANG
# define AlignOf(T) __alignof(T)
#elif COMPILER_GCC
# define AlignOf(T) __alignof__(T)
#else
# error AlignOf not defined for this compiler.
#endif

#define OffsetOf(type, member) offsetof(type, member)
#define AlignPow2(x,b)     (((x) + (b) - 1)&(~((b) - 1)))

#define global        static
#define internal      static 
#define local_persist static 

#define Bit(x) (1u << (x))
#define MaskCheck(flags, mask) cast(bool)(((flags) & (mask)) != 0)
#define MaskSet(var, set, mask) do { \
	if (set) (var) |=  (mask); \
	else     (var) &= ~(mask); \
} while (0)

#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Clamp(lower, x, upper) Min(Max((x), (lower)), (upper))
#define Is_Between(lower, x, upper) (((lower) <= (x)) && ((x) <= (upper)))
#define Abs(x) ((x) < 0 ? -(x) : (x))
#define Lerp(a, b, t) ((a) + ((b) - (a)) * t)

////////////////////////////////
// ~geb: Mem operations

#include <string.h>
#define MemMove(dst, src, size)   memmove((dst), (src), (size))
#define MemZero(dst, size)        memset((dst), 0x00, (size))
#define MemZeroStruct(dst)        memset((dst), 0x00, (sizeof(*dst)))
#define MemCompare(a, b, size)    memcmp((a), (b), (size))
#define MemStrlen(ptr)            (usize) strlen(ptr)

#if COMPILER_MSVC
# define Trap() __debugbreak()
#elif COMPILER_CLANG || COMPILER_GCC
# define Trap() __builtin_trap()
#else
# error Unknown trap intrinsic for this compiler.
#endif

#include <assert.h>
#define AssertAlways(x) assert(x)
#if !defined(NO_ASSERT)
# define Assert(x) AssertAlways(x)
#else
# define Assert(x) ((void)0)
#endif


#if COMPILER_GCC || COMPILER_CLANG

#define ByteSwapU16(x) __builtin_bswap16((u16)(x))
#define ByteSwapU32(x) __builtin_bswap32((u32)(x))
#define ByteSwapU64(x) __builtin_bswap64((u64)(x))

#elif COMPILER_MSVC

force_inline u16 ByteSwapU16(u16 x)
{
    return (u16)((x << 8) | (x >> 8));
}

force_inline u32 ByteSwapU32(u32 x)
{
    return ((x & 0x000000FFu) << 24) |
           ((x & 0x0000FF00u) << 8)  |
           ((x & 0x00FF0000u) >> 8)  |
           ((x & 0xFF000000u) >> 24);
}

force_inline u64 ByteSwapU64(u64 x)
{
    return ((x & 0x00000000000000FFull) << 56) |
           ((x & 0x000000000000FF00ull) << 40) |
           ((x & 0x0000000000FF0000ull) << 24) |
           ((x & 0x00000000FF000000ull) << 8)  |
           ((x & 0x000000FF00000000ull) >> 8)  |
           ((x & 0x0000FF0000000000ull) >> 24) |
           ((x & 0x00FF000000000000ull) >> 40) |
           ((x & 0xFF00000000000000ull) >> 56);
}

#endif

///////////////////////////////////
// ~geb: Allocator

typedef enum AllocationType {
	Allocation_Alloc,
	Allocation_Alloc_Non_Zero,
	Allocation_Free,
	Allocation_FreeAll,
	Allocation_Resize,
	Allocation_Resize_Non_Zero,
} AllocationType;

typedef enum Alloc_Error {
	Alloc_Err_None,
	Alloc_Err_OOM,
	Alloc_Err_Invalid_Pointer,
	Alloc_Err_Invalid_Argument,
	Alloc_Err_Mode_Not_Implemented,
} Alloc_Error;

#define Allocator_Proc(name)                            \
void *name(void *allocator_data, AllocationType type,   \
					 usize size, usize alignment,                 \
					 void *old_memory, usize old_size,            \
					 Alloc_Error *err)
typedef Allocator_Proc(Allocator_Proc);

typedef struct Allocator {
	Allocator_Proc *proc;
	void           *data;
} Allocator;

typedef struct Arena {
	u8 *base;
	usize reserved;
	usize committed;
	usize pos;
} Arena;

typedef struct Arena_Scope {
	Arena *arena;
	usize pos;
} Arena_Scope;

internal Arena_Scope arena_scope_begin(Arena *arena);
internal void arena_scope_end(Arena_Scope scope);


#ifndef DEFAULT_MEMORY_ALIGNMENT
#define DEFAULT_MEMORY_ALIGNMENT cast(usize)(2 * AlignOf(void *))
#endif

force_inline void *
mem_alloc(Allocator a, usize size, bool zero, Alloc_Error *err)
{
	return a.proc(
		a.data,
		zero ? Allocation_Alloc : Allocation_Alloc_Non_Zero,
		size,
		DEFAULT_MEMORY_ALIGNMENT,
		NULL,
		0,
		err
	);
}

force_inline void *
mem_alloc_aligned(Allocator a, usize size, usize alignment, bool zero, Alloc_Error *err)
{
	return a.proc(
		a.data,
		zero ? Allocation_Alloc : Allocation_Alloc_Non_Zero,
		size,
		alignment,
		NULL,
		0,
		err
	);
}

force_inline void
mem_free(Allocator a, void *memory, Alloc_Error *err) {
	a.proc(
		a.data,
		Allocation_Free,
		0,
		0,
		memory,
		0,
		err
	);
}

force_inline void 
mem_free_all(Allocator a) {
	Alloc_Error err;
	a.proc(
		a.data,
		Allocation_FreeAll,
		0,
		0,
		0,
		0,
		&err
	);
}

#define alloc(a, T, _err) cast(T *) mem_alloc_aligned((a), sizeof(T), AlignOf(T), true, _err)
#define alloc_array(a, T, _count, _err) cast(T *) mem_alloc_aligned((a), sizeof(T) * _count, AlignOf(T), true, _err)
#define alloc_array_nz(a, T, _count, _err) cast(T *) mem_alloc_aligned((a), sizeof(T) * _count, AlignOf(T), false, _err)

force_inline void *
mem_resize(Allocator a, void *old_mem, usize old_size, usize new_size, bool zero, Alloc_Error *err) 
{
	return a.proc(
		a.data,
		zero ? Allocation_Resize : Allocation_Resize_Non_Zero,
		new_size,
		DEFAULT_MEMORY_ALIGNMENT,
		old_mem,
		old_size,
		err
	);
}

force_inline void *
mem_resize_aligned(Allocator a, void *old_mem, usize old_size, usize new_size, usize alignment, bool zero, Alloc_Error *err) 
{
	return a.proc(
		a.data,
		zero ? Allocation_Resize : Allocation_Resize_Non_Zero,
		new_size,
		alignment,
		old_mem,
		old_size,
		err
	);
}

internal Allocator heap_allocator(void);
internal Allocator arena_allocator(usize reserve);

///////////////////////////////////
// ~geb: Dynamic Array

typedef struct {
	Allocator alloc;
	void *data;
	usize len;
	usize capacity;
} Dynamic_Array;

#define dynamic_array(_alloc, T, _capacity) \
	(Dynamic_Array){ \
		.alloc    = (_alloc), \
		.data     = (_capacity) ? \
			mem_alloc_aligned((_alloc), sizeof(T) * (_capacity), AlignOf(T), true, NULL) \
			: NULL, \
		.len      = 0, \
		.capacity = (_capacity), \
	}

#define dyn_arr_data(arr, T) ((T *)(arr)->data)

#define dyn_arr_append(arr, T, value) \
	do { \
		if ((arr)->len == (arr)->capacity) { \
			if (!dynamic_array_reserve((arr), sizeof(T), AlignOf(T), (arr)->len + 1)) \
				break; \
		} \
		((T *)(arr)->data)[(arr)->len++] = (value); \
	} while (0)

#define dyn_arr_index(arr, T, i)                         \
    (Assert((i) < (arr)->len),                           \
     ((T*)(arr)->data)[(i)])

internal void dynamic_array_delete(Dynamic_Array *arr);
internal bool dynamic_array_reserve(Dynamic_Array *arr, usize elem_size, usize elem_align, usize min_capacity);
internal void dynamic_array_clear(Dynamic_Array *arr);


///////////////////////////////////
// ~geb: String type ( UTF8 )
// for simplicity it is best to use
// arena allocators for strings

#define RUNE_SELF 0x80
#define RUNE_BOM 0xfeff
#define MAX_RUNE         0x0010ffff // Maximum valid unicode code point
#define REPLACEMENT_CHAR 0xfffd     // Represented an invalid code point
#define MAX_ASCII        0x007f     // Maximum ASCII value
#define MAX_LATIN1       0x00ff     // Maximum Latin-1 value

typedef struct {
	usize len;
	u8 *str;
} String8;

typedef Dynamic_Array String8_List;

global const u8 UTF8_LEN_TABLE[256] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x00–0x0F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x10–0x1F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x20–0x2F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x30–0x3F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x40–0x4F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x50–0x5F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x60–0x6F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x70–0x7F

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x80–0x8F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x90–0x9F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xA0–0xAF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xB0–0xBF

	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // 0xC0–0xCF
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // 0xD0–0xDF
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, // 0xE0–0xEF

	4,4,4,4,4,4,4,4,  // F0–F7
	0,0,0,0,0,0,0,0   // F8–FF (invalid in UTF-8)
};

#define S(x) (String8) { .len = cast(usize) sizeof(x) - 1, .str = cast(u8 *) x }
#define STR "%.*s"
#define s_fmt(s) cast(int) s.len, cast(char *)s.str

internal String8     str8(u8 *ptr, usize len);
internal String8     str8_make(const char *cstring, Allocator allocator);
internal Alloc_Error str8_delete(Allocator alloc, String8 *str);

internal String8     str8_concat(String8 s1, String8 s2, Allocator alloc);

internal isize find_left(String8 str, rune c);
internal isize find_right(String8 str, rune c);

internal String8_List str8_make_list(const char **cstrings, usize count, Allocator allocator);
internal Alloc_Error  str8_delete_list(String8_List *list);
internal String8      str8_list_index(String8_List *list, usize i);

internal String8 str8_slice(String8 string, usize begin, usize end_exclusive);
internal bool    str8_equal(String8 first, String8 second);
internal String8 str8_copy(String8 string, Allocator alloc);
internal String8 str8_copy_cstring(String8 string, Allocator alloc);

internal String8 str8_file_extension(String8 path);
internal String8 str8_file_name(String8 path);

internal String8 str8_tprintf(Allocator alloc, const char *fmt, ...);

typedef struct {
	u8 *ptr;
	u32  width;
	rune codepoint;
} Str_Iterator;

typedef u32 UTF8_Error;
enum {
	UTF8_Err_None,
	UTF8_Err_InvalidLead,
	UTF8_Err_InvalidContinuation,
	UTF8_Err_Overlong,
	UTF8_Err_Surrogate,
	UTF8_Err_OutOfRange
};

internal bool str8_iter(String8 string, Str_Iterator *it);

internal rune utf8_decode(u8 *ptr, UTF8_Error *err);
internal usize utf8_codepoint_size(rune cp);

internal bool is_letter(rune r);
internal bool is_digit(rune r);
internal bool is_space(rune r);

internal bool is_pair_begin(rune r);
internal bool is_pair_end(rune r);
internal String8 get_pair_end(rune r);

///////////////////////////////////
// ~geb: OS layer

// ~geb: heap allocation procs
internal void *os_reserve(usize size);
internal int   os_commit(void *ptr, usize size);
internal void  os_decommit(void *ptr, usize size);
internal void  os_release(void *ptr, usize size);

// ~geb: file handling
typedef i32 OS_Handle;

typedef u32 OS_AccessFlags;
enum {
  OS_AccessFlag_Read       = Bit(0),
  OS_AccessFlag_Write      = Bit(1),
  OS_AccessFlag_Append     = Bit(2),
  OS_AccessFlag_Execute    = Bit(3),
  OS_AccessFlag_ShareRead  = Bit(4),
  OS_AccessFlag_ShareWrite = Bit(5),
};

typedef u32 OS_FileFlags;
enum {
  OS_FileFlag_Directory  = Bit(0),
  OS_FileFlag_ReadOnly   = Bit(1),
  OS_FileFlag_Hidden     = Bit(2),
  OS_FileFlag_System     = Bit(3),
  OS_FileFlag_Archive    = Bit(4),
  OS_FileFlag_Executable = Bit(5),
  OS_FileFlag_Symlink    = Bit(6),
};

typedef u64 OS_Time_Stamp;
typedef struct OS_FileProps {
	usize        size;
	OS_FileFlags flags;
	OS_Time_Stamp created;
	OS_Time_Stamp modified;
} OS_FileProps;

internal OS_Handle    os_stdout();
internal OS_Handle    os_stdin();
internal OS_Handle    os_stderr();

internal OS_Handle    os_file_open(OS_AccessFlags flags, String8 path);
internal void         os_file_close(OS_Handle file);
internal usize        os_file_read(OS_Handle file, usize begin, usize end, void *out_data);
internal usize        os_file_write(OS_Handle file, usize begin, usize end, void *data);
internal OS_FileProps os_properties_from_file(OS_Handle file);

internal String8      os_data_from_path(String8 path, Allocator alloc);
internal bool         os_write_to_path(String8 path, String8 data);

internal bool os_path_exists(String8 path);

// ~geb: time interface

typedef struct OS_Time_Duration {
	f64 seconds;
	f64 milliseconds;
	f64 microseconds;
} OS_Time_Duration;


internal OS_Time_Stamp    os_time_now();
internal OS_Time_Stamp    os_time_frequency();
internal void             os_sleep_ns(u64 ns);
internal OS_Time_Duration os_time_diff(OS_Time_Stamp start, OS_Time_Stamp end);

///////////////////////////////////
// ~geb: Logging

internal void log_debug(const char* fmt, ...);
internal void log_info (const char* fmt, ...);
internal void log_warn (const char* fmt, ...);
internal void log_error(const char* fmt, ...);

#endif
