
function S64
mod_s64(S64 a, S64 b)
{
  S64 r = a % b;
  if (r != 0 && ((r < 0) != (b < 0)))
    r += b;
  return r;
}

function S64
digit_count(S64 n)
{
  S64 count = 1;

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

function U64 
align_up_power2(U64 a, U64 b)
{
  return (a + (b - 1)) & ~(b - 1);
}

function U64
u128_hash(U128 v)
{
  U64 x = v.lo;
  U64 y = v.hi;

  x ^= y + 0x9e3779b97f4a7c15ULL + (x << 6) + (x >> 2);

  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;

  return x;
}

function B32 
u128_matches(U128 a, U128 b)
{
  return a.lo == b.lo && a.hi == b.hi;
}
