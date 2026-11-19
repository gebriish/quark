
////////////////////////////////
// Vec2

function Vec2F32 
vec2_add(Vec2F32 a, Vec2F32 b)
{
  Vec2F32 r = { a.x + b.x, a.y + b.y };
  return r;
}

function Vec2F32 
vec2_sub(Vec2F32 a, Vec2F32 b)
{
  Vec2F32 r = { a.x - b.x, a.y - b.y };
  return r;
}


function Vec2F32 
vec2_mul(Vec2F32 a, Vec2F32 b)
{
  Vec2F32 r = { a.x * b.x, a.y * b.y };
  return r;
}

function Vec2F32 
vec2_scale(Vec2F32 a, F32 v)
{
  Vec2F32 r = { a.x * v, a.y * v };
  return r;
}

////////////////////////////////
// Vec3

function Vec3F32 
vec3_add(Vec3F32 a, Vec3F32 b)
{
  Vec3F32 r = { a.x + b.x, a.y + b.y, a.z + b.z };
  return r;
}

function Vec3F32 
vec3_sub(Vec3F32 a, Vec3F32 b)
{
  Vec3F32 r = { a.x - b.x, a.y - b.y, a.z - b.z };
  return r;
}

function Vec3F32 
vec3_scale(Vec3F32 a, F32 v)
{
  Vec3F32 r = { a.x * v, a.y * v, a.z * v };
  return r;
}

function F32 
vec3_dot(Vec3F32 a, Vec3F32 b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

function Vec3F32 
vec3_cross(Vec3F32 a, Vec3F32 b)
{
  Vec3F32 r = {
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x,
  };
  return r;
}

function Vec3F32 
vec3_normalize(Vec3F32 a)
{
  F32 len = sqrtf(vec3_dot(a, a));
  if(len == 0.f)
  {
    Vec3F32 zero = { 0, 0, 0 };
    return zero;
  }
  return vec3_scale(a, 1.f / len);
}

////////////////////////////////
// Vec4

function Vec4F32 
vec4_add(Vec4F32 a, Vec4F32 b)
{
  Vec4F32 r = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
  return r;
}

function Vec4F32 
vec4_sub(Vec4F32 a, Vec4F32 b)
{
  Vec4F32 r = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
  return r;
}

function Vec4F32 
vec4_scale(Vec4F32 a, F32 v)
{
  Vec4F32 r = { a.x * v, a.y * v, a.z * v, a.w * v };
  return r;
}

////////////////////////////////
// Mat4x4

function Mat4x4F32 
mat4_scalar(F32 v)
{
  Mat4x4F32 r = {0};
  r.m[0][0] = v;
  r.m[1][1] = v;
  r.m[2][2] = v;
  r.m[3][3] = v;
  return r;
}

function Mat4x4F32 
mat4_multiply(Mat4x4F32 a, Mat4x4F32 b)
{
  Mat4x4F32 r = {0};
  for(int c = 0; c < 4; c += 1)
  {
    for(int row = 0; row < 4; row += 1)
    {
      F32 sum = 0.f;
      for(int k = 0; k < 4; k += 1)
      {
        sum += a.m[k][row] * b.m[c][k];
      }
      r.m[c][row] = sum;
    }
  }
  return r;
}

function Mat4x4F32 
mat4_ortho(Rng3F32 box)
{
  F32 l = box.begin[0], r = box.end[0];
  F32 b = box.begin[1], t = box.end[1];
  F32 n = box.begin[2], f = box.end[2];

  Mat4x4F32 m = {0};
  m.m[0][0] =  2.f / (r - l);
  m.m[1][1] =  2.f / (t - b);
  m.m[2][2] = -2.f / (f - n);
  m.m[3][0] = -(r + l) / (r - l);
  m.m[3][1] = -(t + b) / (t - b);
  m.m[3][2] = -(f + n) / (f - n);
  m.m[3][3] =  1.f;
  return m;
}

function Mat4x4F32 
mat4_perspective(F32 fov_y_rad, F32 aspect, F32 z_near, F32 z_far)
{
  F32 f = 1.f / tanf(fov_y_rad * 0.5f);

  Mat4x4F32 m = {0};
  m.m[0][0] = f / aspect;
  m.m[1][1] = f;
  m.m[2][2] = (z_far + z_near) / (z_near - z_far);
  m.m[2][3] = -1.f;
  m.m[3][2] = (2.f * z_far * z_near) / (z_near - z_far);
  return m;
}

function Mat4x4F32 
mat4_look_at(Vec3F32 eye, Vec3F32 target, Vec3F32 up)
{
  Vec3F32 f = vec3_normalize(vec3_sub(target, eye));
  Vec3F32 s = vec3_normalize(vec3_cross(f, up));
  Vec3F32 u = vec3_cross(s, f);

  Mat4x4F32 m = mat4_scalar(1.f);

  m.m[0][0] =  s.x;  m.m[1][0] =  s.y;  m.m[2][0] =  s.z;
  m.m[0][1] =  u.x;  m.m[1][1] =  u.y;  m.m[2][1] =  u.z;
  m.m[0][2] = -f.x;  m.m[1][2] = -f.y;  m.m[2][2] = -f.z;

  m.m[3][0] = -vec3_dot(s, eye);
  m.m[3][1] = -vec3_dot(u, eye);
  m.m[3][2] =  vec3_dot(f, eye);
  return m;
}

function Axis2D 
axis2d_flip(Axis2D axis)
{
  Assert(axis < 2);
  return (1 - axis);
}

