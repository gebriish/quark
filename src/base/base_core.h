#ifndef BASE_CORE_H
#define BASE_CORE_H

#include "base_context.h"

////////////////////////////////
// ~geb: Foreign Includes

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

////////////////////////////////
// ~geb: Useful macros

#define internal_lnk  static
#define global        static
#define local_persist static

#define KB(n)  (((usize)(n)) << 10)
#define MB(n)  (((usize)(n)) << 20)
#define GB(n)  (((usize)(n)) << 30)
 
#define Min(A,B) (((A)<(B))?(A):(B))
#define Max(A,B) (((A)>(B))?(A):(B))
#define Clamp(A,X,B) (((X)<(A))?(A):((X)>(B))?(B):(X))

#if COMPILER_MSVC
# define AlignOf(T) __alignof(T)
#elif COMPILER_CLANG
# define AlignOf(T) __alignof(T)
#elif COMPILER_GCC
# define AlignOf(T) __alignof__(T)
#else
# error AlignOf not defined for this compiler.
#endif

#define ByteSwapU32(x) (             \
  (((x) & (u32)0x000000FFu) << 24) | \
  (((x) & (u32)0x0000FF00u) << 8)  | \
  (((x) & (u32)0x00FF0000u) >> 8)  | \
  (((x) & (u32)0xFF000000u) >> 24) )

#define OffsetOf(type, member) ((usize) &(((type *)0)->member))

#if COMPILER_CLANG || COMPILER_GCC
# define force_inline inline __attribute__((always_inline))
#elif COMPILER_MSVC
# define force_inline __forceinline
#else
# define force_inline inline
#endif

#define Glue(A,B) A##B

////////////////////////////////
// ~geb: Mem operations

#define MemCopy(dst, src, size)  memmove((dst), (src), (size))
#define MemZero(dst, size)        memset((dst), 0x00, (size))
#define MemCompare(a, b, size)    memcmp((a), (b), (size))
#define MemStrlen(ptr)            strlen(ptr)

#if COMPILER_MSVC
# define Trap() __debugbreak()
#elif COMPILER_CLANG || COMPILER_GCC
# define Trap() __builtin_trap()
#else
# error Unknown trap intrinsic for this compiler.
#endif

#define AssertAlways(x) do{if(!(x)) {Trap();}}while(0)
#if DEBUG_BUILD
# define Assert(x) AssertAlways(x)
#else
# define Assert(x) (void)(x)
#endif

#define StaticAssert(expr, str) _Static_assert(expr, str)

#define AlignPow2(x,b)     (((x) + (b) - 1)&(~((b) - 1)))

////////////////////////////////
// ~geb: Type shorthands

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef i8       bool;
#define true    ((bool)1)
#define false   ((bool)0)

typedef float    f32;
typedef double   f64;
StaticAssert(sizeof(f32) == 4, "float32 size not correct");
StaticAssert(sizeof(f64) == 8, "float64 size not correct");

#if ARCH_64BIT
typedef u64 usize;
typedef i64 isize;
#else
typedef u32 usize;
typedef i32 isize;
#endif

typedef struct {
  f32 x, y;
} vec2_f32;

typedef union {
  struct { f32 x, y, z, w; };
  struct { f32 r, g, b, a; };
} vec4_f32;

#if DEBUG_BUILD
# define _log_base(stream, level, fmt, ...)                       \
  do {                                                           \
    fprintf(stream, "[%s] %s:%d (%s): " fmt "\n",                \
            level, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
  } while (0)

# define LogError(fmt, ...)  _log_base(stderr, "ERROR", fmt, ##__VA_ARGS__)
# define LogWarn(fmt,  ...)  _log_base(stderr, "WARN ", fmt, ##__VA_ARGS__)
# define LogInfo(fmt,  ...)  _log_base(stdout, "INFO ", fmt, ##__VA_ARGS__)
# define LogDebug(fmt, ...)  _log_base(stdout, "DEBUG", fmt, ##__VA_ARGS__)
#else
# define LogError(fmt, ...)   ((void)0)
# define LogWarn(fmt, ...)    ((void)0)
# define LogInfo(fmt, ...)    ((void)0)
# define LogDebug(fmt, ...)   ((void)0)
#endif

////////////////////////////////
// ~geb: Arena Allocator 

#define ARENA_HEADER_SIZE sizeof(Arena)

typedef struct Arena Arena;
struct Arena {
  usize last_used;
  usize used;
  usize capacity;
};

typedef struct Temp Temp;
struct Temp {
  Arena *arena;
  usize pos;
};

internal_lnk Arena *arena_alloc(usize capacity);
internal_lnk void   arena_release(Arena *arena);

internal_lnk void  *arena_push(Arena *arena, usize size, usize align, bool zero);
internal_lnk void   arena_pop(Arena *arena);
internal_lnk void   arena_pop_to(Arena *arena, usize pos);
internal_lnk void   arena_clear(Arena *arena);
internal_lnk usize  arena_pos(Arena *arena);

internal_lnk Temp   temp_begin(Arena *arena);
internal_lnk void   temp_end(Temp temp);

#define arena_push_struct(a, T)   (T *) arena_push((a), sizeof(T), AlignOf(T), true)
#define arena_push_array(a, T, c) (T *) arena_push((a), sizeof(T) * (c), AlignOf(T), false)

#endif
