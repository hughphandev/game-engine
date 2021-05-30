#ifndef JUSA_MATH_CPP
#define JUSA_MATH_CPP

#include "jusa_math.h"
#include "jusa_utils.h"
#include <intrin.h>

//TODO: Use math.h for now. Switch to platform efficient code in the future!
#include <math.h>

inline float Floor(float value)
{
  return floorf(value);
}
inline float Ceil(float value)
{
  return ceilf(value);
}

inline float Round(float value)
{
  return roundf(value);
}

inline float Sin(float value)
{
  return sinf(value);
}
inline float Cos(float value)
{
  return cosf(value);
}
inline float Tan(float value)
{
  return tanf(value);
}
inline float Atan2(float y, float x)
{
  return atan2f(y, x);
}
inline float Atan(float value)
{
  return atanf(value);
}

inline float Q_rsqrt(float value)
{
  long i;
  float x2, y;
  const float threeHalfs = 1.5f;

  x2 = value * 0.5f;
  y = value;
  i = *(long*)&y;
  i = 0x5f3759df - (i >> 1);
  y = *(float*)&i;

  //NOTE: newton iteration, duplicate to increase precision!
  y = y * (threeHalfs - (x2 * y * y));

  return y;
}

inline float Sqr(float x)
{
  return (x * x);
}

inline float Abs(float value)
{
  u32 bit = *(u32*)&value;
  bit = bit & (0x7FFFFFFF);
  value = *(float*)&bit;
  return value;
}

inline v2 Round(v2 value)
{
  return { Round(value.x), Round(value.y) };
}

inline float Lerp(float a, float b, float t)
{
  float delta = b - a;
  return a + (delta * t);
}

inline float Min(float a, float b)
{
  return a < b ? a : b;
}

inline u32 RoundToU32(float value)
{
  return (u32)(value + 0.5f);
}

inline float Max(float a, float b)
{
  return a > b ? a : b;
}

inline v2 V2(float x, float y)
{
  return { x, y };
}

inline v2 v2::Normalize()
{
  if (*this == 0.0f) return { 0.0f, 0.0f };
  float rsqrtMagtitude = Q_rsqrt(Sqr(this->x) + Sqr(this->y));
  return{ this->x * rsqrtMagtitude, this->y * rsqrtMagtitude };
}

inline float Inner(v2 a, v2 b)
{
  return (a.x * b.x + a.y * b.y);
}

inline v2 operator*(v2 a, float b)
{
  return { a.x * b, a.y * b };
}
inline v2 operator*(float a, v2 b)
{
  return b * a;
}
inline v2 operator/(v2 a, float b)
{
  return { a.x / b, a.y / b };
}
inline v2 operator+(v2 a, float b)
{
  return { a.x + b, a.y + b };
}
inline v2 operator-(v2 a, float b)
{
  return { a.x - b, a.y - b };
}

inline v2 operator*(v2 a, v2 b)
{
  return { a.x * b.x, a.y * b.y };
}
inline v2 operator/(v2 a, v2 b)
{
  return { a.x / b.x, a.y / b.y };
}
inline v2 operator+(v2 a, v2 b)
{
  return { a.x + b.x, a.y + b.y };
}
inline v2 operator-(v2 a, v2 b)
{
  return { a.x - b.x, a.y - b.y };
}

inline v2 v2::operator*=(v2 a)
{
  return { this->x *= a.x, this->y *= a.y };
}
inline v2 v2::operator/=(v2 a)
{
  return { this->x /= a.x, this->y /= a.y };
}
inline v2 v2::operator+=(v2 a)
{
  return { this->x += a.x, this->y += a.y };
}
inline v2 v2::operator-=(v2 a)
{
  return { this->x -= a.x, this->y -= a.y };
}
inline bool v2::operator==(v2 a)
{
  return (this->x == a.x && this->y == a.y);
}
inline bool v2::operator!=(v2 a)
{
  return (this->x != a.x || this->y != a.y);
}
inline bool v2::operator<=(v2 a)
{
  return (this->x <= a.x && this->y <= a.y);
}
inline bool v2::operator<(v2 a)
{
  return (this->x < a.x&& this->y < a.y);
}
inline bool v2::operator>(v2 a)
{
  return (this->x > a.x && this->y > a.y);
}

inline v2 v2::operator=(float a)
{
  return { a, a };
}
inline v2 v2::operator*=(float a)
{
  return { this->x *= a, this->y *= a };
}
inline v2 v2::operator/=(float a)
{
  return { this->x /= a, this->y /= a };
}
inline v2 v2::operator+=(float a)
{
  return { this->x += a, this->y += a };
}
inline v2 v2::operator-=(float a)
{
  return { this->x -= a, this->y -= a };
}
inline bool v2::operator==(float a)
{
  return (this->x == a && this->y == a);
}
inline bool v2::operator!=(float a)
{
  return (this->x != a || this->y != a);
}
inline bool v2::operator<=(float a)
{
  return (this->x <= a && this->y <= a);
}
inline bool v2::operator<(float a)
{
  return (this->x < a&& this->y < a);
}
inline bool v2::operator>(float a)
{
  return (this->x > a && this->y > a);
}

inline v2 Abs(v2 v)
{
  return { Abs(v.x), Abs(v.y) };
}

#endif