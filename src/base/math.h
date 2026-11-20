#ifndef BASE_MATH_H
#define BASE_MATH_H

////////////////////////////////////
// @section: 2 Vectors

typedef union Vec2S16 Vec2S16;
union Vec2S16
{
  struct { s16 x, y; };
  s16 xy[2];
};

typedef union Vec2S32 Vec2S32;
union Vec2S32
{
  struct { s32 x, y; };
  s32 xy[2];
};

typedef union Vec2F32 Vec2F32;
union Vec2F32
{
  struct { f32 x, y; };
  f32 xy[2];
};

#define v2s16(...) (Vec2S16) { __VA_ARGS__ }
#define v2s32(...) (Vec2S32) { __VA_ARGS__ }
#define v2f32(...) (Vec2F32) { __VA_ARGS__ }

function Vec2F32 vec2_add(Vec2F32 a, Vec2F32 b);
function Vec2F32 vec2_sub(Vec2F32 a, Vec2F32 b);
function Vec2F32 vec2_mul(Vec2F32 a, Vec2F32 b);
function Vec2F32 vec2_scale(Vec2F32 a, f32 v);

////////////////////////////////////
// @section: 3 Vectors

typedef union Vec3F32 Vec3F32;
union Vec3F32
{
  struct { f32 x, y, z; };
  f32 xyz[3];
};

#define v3f32(...) (Vec3F32) { __VA_ARGS__ }
function Vec3F32 vec3_add(Vec3F32 a, Vec3F32 b);
function Vec3F32 vec3_sub(Vec3F32 a, Vec3F32 b);
function Vec3F32 vec3_scale(Vec3F32 a, f32 v);
function f32     vec3_dot(Vec3F32 a, Vec3F32 b);
function Vec3F32 vec3_cross(Vec3F32 a, Vec3F32 b);
function Vec3F32 vec3_normalize(Vec3F32 a);

////////////////////////////////////
// @section: 4 Vectors

typedef union Vec4F32 Vec4F32;
union Vec4F32
{
  struct { f32 x, y, z, w; };
  struct { f32 r, g, b, a; };
  f32 xyzw[4];
  f32 rgba[4];
};

#define v4f32(...) (Vec4F32) { __VA_ARGS__ }
function Vec4F32 vec4_add(Vec4F32 a, Vec4F32 b);
function Vec4F32 vec4_sub(Vec4F32 a, Vec4F32 b);
function Vec4F32 vec4_scale(Vec4F32 a, f32 v);

////////////////////////////////////
// @section: Util contructs

typedef u32 Pivot;
enum
{
  Pivot_TopLeft, Pivot_TopCenter, Pivot_TopRight,
  Pivot_MidLeft, Pivot_MidCenter, Pivot_MidRight,
  Pivot_LowLeft, Pivot_LowCenter, Pivot_LowRight,
  Pivot_COUNT,
};

global read_only Vec2F32 pivot_offsets[Pivot_COUNT] = 
{
  { 0.f, 0.f }, { .5f, 0.f }, { 1.f, 0.f },
  { 0.f, .5f }, { .5f, .5f }, { 1.f, .5f },
  { 0.f, 1.f }, { .5f, 1.f }, { 1.f, 1.f },
};

typedef u32 Corner;
enum
{
  Corner_TopLeft, Corner_TopRight,
  Corner_LowRight, Corner_LowLeft,
  Corner_COUNT,
};

typedef u32 Edge;
enum
{
  Edge_Top, Edge_Right, 
  Edge_Bottom, Edge_Left,
  Edge_COUNT,
};

typedef u32 Axis2D;
enum
{
  Axis2D_Horizontal = 0,
  Axis2D_Vertical   = 1,
};

typedef struct Rng1F32 Rng1F32;
struct Rng1F32
{
  f32 begin;
  f32 end;
};

typedef struct Rng1S64 Rng1S64;
struct Rng1S64
{
  s64 begin;
  s64 end;
};

typedef union Rng2S32 Rng2S32;
union Rng2S32
{
  struct 
  {
    s32 begin[2];
    s32   end[2];
  };
  struct
  {
    Vec2S32 min;
    Vec2S32 max;
  };
};

typedef union Rng2S16 Rng2S16;
union Rng2S16
{
  struct 
  {
    s16 begin[2];
    s16   end[2];
  };
  struct 
  {
    Vec2S16 min;
    Vec2S16 max;
  };
};

typedef union Rng2F32 Rng2F32;
union Rng2F32
{
  struct
  {
    Vec2F32 min;
    Vec2F32 max;
  };
  struct
  {
    f32 begin[2];
    f32   end[2];
  };
};

typedef union Rng3F32 Rng3F32;
union Rng3F32
{
  struct
  {
    Vec3F32 min;
    Vec3F32 max;
  };
  struct
  {
    f32 begin[3];
    f32   end[3];
  };
};

#define Rng1Check(v, rng) \
  ((v) >= (rng).begin && (v) <= (rng).end)
#define Rng2Check(v, rng) \
  ((v)[0] >= (rng).begin[0] && (v)[0] <= (rng).end[0] && \
   (v)[1] >= (rng).begin[1] && (v)[1] <= (rng).end[1])
#define Rng3Check(v, rng) \
  ((v)[0] >= (rng).begin[0] && (v)[0] <= (rng).end[0] && \
   (v)[1] >= (rng).begin[1] && (v)[1] <= (rng).end[1] && \
   (v)[2] >= (rng).begin[2] && (v)[2] <= (rng).end[2])

function Axis2D axis2d_flip(Axis2D axis);

////////////////////////////////////
// @section: 4x4 Matrix

typedef union Mat4x4F32 Mat4x4F32;
union Mat4x4F32
{
  Vec4F32 cols[4];
  f32     m[4][4];
  f32     raw[16];
};

function Mat4x4F32 mat4_scalar(f32 v);
function Mat4x4F32 mat4_multiply(Mat4x4F32 a, Mat4x4F32 b);
function Mat4x4F32 mat4_ortho(Rng3F32 box);
function Mat4x4F32 mat4_perspective(f32 fov_y_rad, f32 aspect, f32 z_near, f32 z_far);
function Mat4x4F32 mat4_look_at(Vec3F32 eye, Vec3F32 target, Vec3F32 up);

#endif // BASE_MATH_H
