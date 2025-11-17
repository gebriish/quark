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

#define internal      static
#define global        static
#define local_persist static

#define Enum(name, type) typedef type name; enum

#define KB(n)  (((usize)(n)) << 10)
#define MB(n)  (((usize)(n)) << 20)
#define GB(n)  (((usize)(n)) << 30)

#define Min(A,B) (((A)<(B))?(A):(B))
#define Max(A,B) (((A)>(B))?(A):(B))
#define Clamp(A,X,B) (((X)<(A))?(A):((X)>(B))?(B):(X))
#define Lerp(a, b, t) ((a) + ((b) - (a)) * t)

#define Abs_i64(v) (i64)llabs(v)

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

#define OffsetOf(s,m) ((size_t)&(((s*)0)->m))

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

#define MemMove(dst, src, size)  memmove((dst), (src), (size))
#define MemZero(dst, size)        memset((dst), 0x00, (size))
#define MemZeroStruct(dst)        memset((dst), 0x00, (sizeof(*dst)))
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
#define NoOp               ((void)0)

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

#define U8_MAX  0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFF

#define I8_MIN  (-128)
#define I8_MAX  127
#define I16_MIN (-32768)
#define I16_MAX 32767
#define I32_MIN (-2147483648)
#define I32_MAX 2147483647
#define I64_MIN (-9223372036854775807LL - 1)
#define I64_MAX 9223372036854775807LL

StaticAssert(sizeof(f32) == 4, "float32 size not correct");
StaticAssert(sizeof(f64) == 8, "float64 size not correct");

#if ARCH_64BIT
typedef u64 usize;
typedef i64 isize;
#else
typedef u32 usize;
typedef i32 isize;
#endif

////////////////////////////////
// ~geb: Math 

#define VecDef(N, T, ...) \
typedef struct {\
	T __VA_ARGS__;\
} vec##N##_##T

VecDef(2, f32, x, y);

VecDef(2, i32, x, y);
VecDef(2, u32, x, y);
VecDef(2, i16, x, y);
VecDef(2, u16, x, y);
VecDef(2, i8,  x, y);
VecDef(2, u8,  x, y);

VecDef(4, f32, x, y, z, w);
VecDef(4, u16, x, y, z, w);

internal force_inline f32 
smooth_damp(f32 current, f32 target, f32 time, f32 dt)
{
	if (dt <= 0 || time <= 0) return target;

	f32 rate = 2.0f / time;
	f32 x = rate * dt;

	f32 factor = 0;
	if (x < 0.0001f) {
		factor = x * (1.0f - x*0.5f + x*x/6.0f - x*x*x/24.0f);
	} else {
		factor = 1.0f - expf(-x);
	}

	return Lerp(current, target, factor);
}

#if DEBUG_BUILD
# define _log_base(stream, level, fmt, ...)                    \
do {                                                           \
	fprintf(stream, "[%s] %s:%d (%s): " fmt "\n",                \
		 level, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
} while (0)

# define LogError(fmt, ...)  _log_base(stderr, "ERROR", fmt, ##__VA_ARGS__)
# define LogWarn(fmt,  ...)  _log_base(stderr, "WARN ", fmt, ##__VA_ARGS__)
# define LogInfo(fmt,  ...)  _log_base(stdout, "INFO ", fmt, ##__VA_ARGS__)
# define LogDebug(fmt, ...)  _log_base(stdout, "DEBUG", fmt, ##__VA_ARGS__)

typedef struct Source_Code_Location Source_Code_Location;
struct Source_Code_Location {
	const char *file_path;
	const char *procedure;
	int line, column;
};

#else
# define LogError(fmt, ...)   ((void)0)
# define LogWarn(fmt, ...)    ((void)0)
# define LogInfo(fmt, ...)    ((void)0)
# define LogDebug(fmt, ...)   ((void)0)
#endif


#define DeferScope(begin, end) for(int _i_  = ((begin), 0); !_i_; _i_ += 1, (end))

#endif
