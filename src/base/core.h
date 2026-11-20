#ifndef BASE_CORE_H
#define BASE_CORE_H

//////////////////////////
// @section: compiler context

#if defined(__clang__)
# define COMPILER_CLANG 1
#elif defined(__GNUC__)
# define COMPILER_GCC 1
#elif defined(_MSC_VER)
# define COMPILER_CL 1
#else
# error "unsupported compiler"
#endif

#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CL)
# define COMPILER_CL 0
#endif

//////////////////////////
// @section: OS context

#if defined(_WIN32)
# define OS_WINDOWS 1
#elif defined(__linux__)
# define OS_LINUX 1
#elif defined(__APPLE__)
# define OS_MAC 1
#else
# error "unsupported OS"
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

///////////////////////////
// @section: custom keywords

#if COMPILER_CL
# define force_inline __forceinline
#elif COMPILER_CLANG || COMPILER_GCC
# define force_inline static inline __attribute__((always_inline))
#else
# define force_inline static inline
#endif

#if COMPILER_CL || (COMPILER_CLANG && OS_WINDOWS)
# pragma section(".rdata$", read)
# define read_only __declspec(allocate(".rdata$"))
#elif COMPILER_CLANG
# define read_only __attribute__((section(".rodata")))
#else
# define read_only
#endif

#if COMPILER_CL
# define thread_static __declspec(thread)
#elif COMPILER_CLANG || COMPILER_GCC
# define thread_static __thread
#endif

#define function static
#define global   static

///////////////////////////
// @section: base includes

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

///////////////////////////
// @section: builtin basic types

typedef uint8_t  u8;
typedef  int8_t  s8;
typedef uint16_t u16;
typedef  int16_t s16;
typedef uint32_t u32;
typedef  int32_t s32;
typedef uint64_t u64;
typedef  int64_t s64;

typedef union u128 u128;
union u128
{
  struct
  {
    u64 lo;
    u64 hi;
  };
  struct
  {
    u32 lo32[2];
    u32 hi32[2];
  };
  struct
  {
    u16 lo16[4];
    u16 hi16[4];
  };
};

typedef float  f32;
typedef double f64;

typedef u8  b8;
typedef u32 b32;

#define true  1
#define false 0

///////////////////////////
// @section: base macros

#define CArrayLength(x) sizeof(x) / sizeof(x[0])

#define DeferLoop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

#define EachIndex(itr, n) (s64 (itr)=0; (itr) < (s64)(n); ++(itr))
#define EachRange(itr, b, e) (s64 (itr)=b; (itr) <= (s64)(e); ++(itr))
#define EachNode(it, T, first) (T *it = first; it != 0; it = it->next)

#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Clamp(min, v, max) Max(min, Min(v, max))
#define Lerp(_curr, _tar, _t) ((_curr) + ((_tar) - (_curr)) * (_t))

#define Kilobytes(x) (u64) ((u64)(x) << 10)
#define Megabytes(x) (u64) ((u64)(x) << 20)
#define Gigabytes(x) (u64) ((u64)(x) << 30)

#define MemMove(dst, src, len) memmove((dst), (src), (size_t) (len))
#define MemZero(dst, len)      memset((dst), 0x0, (size_t) (len))
#define MemZeroStruct(ptr)     MemZero(ptr, sizeof(*ptr))

#if COMPILER_CL
# define align_of(T) __alignof(T)
#elif COMPILER_CLANG
# define align_of(T) __alignof(T)
#elif COMPILER_GCC
# define align_of(T) __alignof__(T)
#else
# error align_of not defined for this compiler.
#endif

/*
  @note(gaureesh): slice should look like this.
  
  struct
  {
    T     *raw;
    S64   len;
  }
  
  @note(gaureesh): arrays ( growable )  should look like this
  
  struct
  {
    T      *raw;
    S64    capacity;
    S64    len;
  }
*/

#define Append(_a, _v)                                    \
do {                                                      \
  if ((_a)->len < (_a)->capacity)                         \
    (_a)->raw[(_a)->len++] = (_v);                        \
  else                                                    \
    fprintf(stderr, "growable array capacity exceeded\n");\
} while (0)

///////////////////////////////
// @section: singly linked singly headed

#define SLLStackPush_N(f,n,next) \
do {                             \
  (n)->next = (f);               \
  (f) = (n);                     \
} while (0)

#define SLLStackPop_N(f,next)  \
do {                           \
  (f) = (f)->next;             \
} while (0)

#define SLLStackPush(f,n)        SLLStackPush_N(f,n,next)
#define SLLStackPop(f)           SLLStackPop_N(f,next)

///////////////////////////////
// @section: singly linked double headed

#define SLLQueuePush_N(f,l,n,next) \
do {                               \
  (n)->next = 0;                   \
  if (l)                           \
    (l)->next = (n);               \
  else                             \
    (f) = (n);                     \
  (l) = (n);                       \
} while (0)

#define SLLQueuePushFront_N(f,l,n,next)  \
do {                                     \
  (n)->next = (f);                       \
  if (!(f))                              \
    (l) = (n);                           \
  (f) = (n);                             \
} while (0)

#define SLLQueuePop_N(f,l,next)          \
do {                                     \
  if (f)                                 \
  {                                      \
    (f) = (f)->next;                     \
    if (!(f))                            \
      (l) = 0;                           \
  }                                      \
} while (0)

#define SLLQueuePush(f,l,n)      SLLQueuePush_N(f,l,n,next)
#define SLLQueuePushFront(f,l,n) SLLQueuePushFront_N(f,l,n,next)
#define SLLQueuePop(f,l)         SLLQueuePop_N(f,l,next)

///////////////////////////////
// @section: doubly linked double headed

#define DLLPushFront_N(f,l,n,next,prev) \
do {                                    \
  (n)->prev = 0;                        \
  (n)->next = (f);                      \
  if (f)                                \
    (f)->prev = (n);                    \
  else                                  \
    (l) = (n);                          \
  (f) = (n);                            \
} while (0)

#define DLLPushBack_N(f,l,n,next,prev)  \
do {                                    \
  (n)->next = 0;                        \
  (n)->prev = (l);                      \
  if (l)                                \
    (l)->next = (n);                    \
  else                                  \
    (f) = (n);                          \
  (l) = (n);                            \
} while (0)

#define DLLPopFront_N(f,l,next,prev)    \
do {                                    \
  if (f)                                \
  {                                     \
    (f) = (f)->next;                    \
    if (f)                              \
      (f)->prev = 0;                    \
    else                                \
      (l) = 0;                          \
  }                                     \
} while (0)

#define DLLPopBack_N(f,l,next,prev)     \
do {                                    \
  if (l)                                \
  {                                     \
    (l) = (l)->prev;                    \
    if (l)                              \
      (l)->next = 0;                    \
    else                                \
      (f) = 0;                          \
  }                                     \
} while (0)

#define DLLRemove_N(f,l,n,next,prev)    \
do {                                    \
  if ((n)->prev)                        \
    (n)->prev->next = (n)->next;        \
  else                                  \
    (f) = (n)->next;                    \
  if ((n)->next)                        \
    (n)->next->prev = (n)->prev;        \
  else                                  \
    (l) = (n)->prev;                    \
} while (0)

#define DLLPushFront(f,l,n)      DLLPushFront_N(f,l,n,next,prev)
#define DLLPushBack(f,l,n)       DLLPushBack_N(f,l,n,next,prev)
#define DLLPopFront(f,l)         DLLPopFront_N(f,l,next,prev)
#define DLLPopBack(f,l)          DLLPopBack_N(f,l,next,prev)
#define DLLRemove(f,l,n)         DLLRemove_N(f,l,n,next,prev)

///////////////////////////////
// @section: helper procs

function s64 mod_s64(s64 a, s64 b);         // mathematical a mod b
function s64 digit_count(s64 n);            // number of digits in any signed integer
function u64 align_up_power2(u64 a, u64 b); // aligns a to nearest multiple of a 2's power b
function u64 u128_hash(u128 v);
function b32 u128_matches(u128 a, u128 b);

#endif // BASE_CORE_H
