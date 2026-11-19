#ifndef BASE_MATH_H
#define BASE_MATH_H

////////////////////////////////////
// @section: math types

typedef union Vec2S16 Vec2S16;
union Vec2S16
{
  struct { S16 x, y; };
  S16 xy[2];
};

typedef union Vec2S32 Vec2S32;
union Vec2S32
{
  struct { S32 x, y; };
  S32 xy[2];
};

typedef union Vec2F32 Vec2F32;
union Vec2F32
{
  struct { F32 x, y; };
  F32 xy[2];
};

typedef union Vec3F32 Vec3F32;
union Vec3F32
{
  struct { F32 x, y, z; };
  F32 xyz[3];
};

typedef union Vec4F32 Vec4F32;
union Vec4F32
{
  struct { F32 x, y, z, w; };
  struct { F32 r, g, b, a; };
  F32 xyzw[4];
  F32 rgba[4];
};

typedef union Mat4x4F32 Mat4x4F32;
union Mat4x4F32
{
  Vec4F32 cols[4];
  F32     m[4][4];
  F32     raw[16];
};

typedef U32 Pivot;
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

typedef U32 Corner;
enum
{
  Corner_TopLeft, Corner_TopRight,
  Corner_LowRight, Corner_LowLeft,
  Corner_COUNT,
};

typedef U32 Edge;
enum
{
  Edge_Top, Edge_Right, 
  Edge_Bottom, Edge_Left,
  Edge_COUNT,
};

typedef U32 Axis2D;
enum
{
  Axis2D_Horizontal = 0,
  Axis2D_Vertical   = 1,
};

typedef struct Rng1F32 Rng1F32;
struct Rng1F32
{
  F32 min;
  F32 max;
};

typedef struct Rng1S64 Rng1S64;
struct Rng1S64
{
  S64 min;
  S64 max;
};

typedef union Rng2S32 Rng2S32;
union Rng2S32
{
  struct 
  {
    S32 begin[2];
    S32   end[2];
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
    S16 begin[2];
    S16   end[2];
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
    F32 begin[2];
    F32   end[2];
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
    F32 begin[3];
    F32   end[3];
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

////////////////////////////////////
// @section: constructors

#define V2(...) (Vec2F32) { __VA_ARGS__ }
#define V3(...) (Vec3F32) { __VA_ARGS__ }
#define V4(...) (Vec4F32) { __VA_ARGS__ }

#define V2_Params(v) (v).x, (v).y
#define V3_Params(v) (v).x, (v).y, (v).z
#define V4_Params(v) (v).x, (v).y, (v).z, (v).w

////////////////////////////////////
// @section: vector math functions

function Vec2F32 vec2_add(Vec2F32 a, Vec2F32 b);
function Vec2F32 vec2_sub(Vec2F32 a, Vec2F32 b);
function Vec2F32 vec2_mul(Vec2F32 a, Vec2F32 b);
function Vec2F32 vec2_scale(Vec2F32 a, F32 v);

function Vec3F32 vec3_add(Vec3F32 a, Vec3F32 b);
function Vec3F32 vec3_sub(Vec3F32 a, Vec3F32 b);
function Vec3F32 vec3_scale(Vec3F32 a, F32 v);
function F32     vec3_dot(Vec3F32 a, Vec3F32 b);
function Vec3F32 vec3_cross(Vec3F32 a, Vec3F32 b);
function Vec3F32 vec3_normalize(Vec3F32 a);

function Vec4F32 vec4_add(Vec4F32 a, Vec4F32 b);
function Vec4F32 vec4_sub(Vec4F32 a, Vec4F32 b);
function Vec4F32 vec4_scale(Vec4F32 a, F32 v);

function Mat4x4F32 mat4_scalar(F32 v);
function Mat4x4F32 mat4_multiply(Mat4x4F32 a, Mat4x4F32 b);
function Mat4x4F32 mat4_ortho(Rng3F32 box);
function Mat4x4F32 mat4_perspective(F32 fov_y_rad, F32 aspect, F32 z_near, F32 z_far);
function Mat4x4F32 mat4_look_at(Vec3F32 eye, Vec3F32 target, Vec3F32 up);

///////////////////////////////
// @section utilities

function Axis2D axis2d_flip(Axis2D axis);

#endif // BASE_MATH_H
