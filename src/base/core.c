
function s64
mod_s64(s64 a, s64 b)
{
  s64 r = a % b;
  if (r != 0 && ((r < 0) != (b < 0)))
    r += b;
  return r;
}

function s64
digit_count(s64 n)
{
  s64 count = 1;

  if (n < 0) {
    count += 1;
    n = -(n + 1);
  }

  while (n >= 10) {
    n /= 10;
    count += 1;
  }

  return count;
}

function u64 
align_up_power2(u64 a, u64 b)
{
  return (a + (b - 1)) & ~(b - 1);
}

function u64
u128_hash(u128 v)
{
  u64 x = v.lo;
  u64 y = v.hi;

  x ^= y + 0x9e3779b97f4a7c15ULL + (x << 6) + (x >> 2);

  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;

  return x;
}

function b32 
u128_matches(u128 a, u128 b)
{
  return a.lo == b.lo && a.hi == b.hi;
}
