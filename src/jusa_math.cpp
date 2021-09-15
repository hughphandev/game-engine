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
  return (1 - t) * a + t * b;
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
  return *this = (*this * a);
}
inline v2 v2::operator/=(v2 a)
{
  return *this = (*this / a);
}
inline v2 v2::operator+=(v2 a)
{
  return *this = (*this + a);
}
inline v2 v2::operator-=(v2 a)
{
  return *this = (*this - a);
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

inline v2 v2::operator*=(float a)
{
  return *this = (*this * a);
}
inline v2 v2::operator/=(float a)
{
  return *this = (*this / a);
}
inline v2 v2::operator+=(float a)
{
  return *this = (*this + a);
}
inline v2 v2::operator-=(float a)
{
  return *this = (*this - a);
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
inline v2 v2::operator-()
{
  return { -this->x, -this->y };
}

inline v2 Abs(v2 v)
{
  return { Abs(v.x), Abs(v.y) };
}

inline v2 Perp(v2 v)
{
  return { -v.y, v.x };
}

inline v3 v3::operator*=(v3 a)
{
  return *this = (*this * a);
}
inline v3 v3::operator/=(v3 a)
{
  return *this = (*this / a);
}
inline v3 v3::operator+=(v3 a)
{
  return *this = (*this + a);
}
inline v3 v3::operator-=(v3 a)
{
  return *this = (*this - a);
}
inline bool v3::operator==(v3 a)
{
  return (this->x == a.x && this->y == a.y && this->z == a.z);
}
inline bool v3::operator!=(v3 a)
{
  return !(*this == a);
}
inline bool v3::operator<=(v3 a)
{
  return (this->x <= a.x && this->y <= a.y && this->z <= a.z);
}
inline bool v3::operator<(v3 a)
{
  return (this->x < a.x&& this->y < a.y&& this->z < a.z);
}
inline bool v3::operator>(v3 a)
{
  return (this->x > a.x && this->y > a.y && this->z > a.z);
}

inline v3 v3::operator*=(float a)
{
  return *this = (*this * a);
}
inline v3 v3::operator/=(float a)
{
  return *this = (*this / a);
}
inline v3 v3::operator+=(float a)
{
  return *this = (*this + a);
}
inline v3 v3::operator-=(float a)
{
  return *this = (*this - a);
}
inline bool v3::operator==(float a)
{
  return (this->x == a && this->y == a && this->z == a);
}
inline bool v3::operator!=(float a)
{
  return !(*this == a);
}
inline bool v3::operator<=(float a)
{
  return (this->x <= a && this->y <= a && this->z <= a);
}
inline bool v3::operator<(float a)
{
  return (this->x < a&& this->y < a&& this->z < a);
}
inline bool v3::operator>(float a)
{
  return (this->x > a && this->y > a && this->z > a);
}
inline v3 v3::operator-()
{
  return { -this->x, -this->y, -this->z };
}

inline v3 operator*(v3 a, float b)
{
  return { a.x * b, a.y * b, a.z * b };
}
inline v3 operator*(float a, v3 b)
{
  return b * a;
}
inline v3 operator/(v3 a, float b)
{
  return { a.x / b, a.y / b, a.z / b };
}
inline v3 operator+(v3 a, float b)
{
  return { a.x + b, a.y + b, a.z + b };
}
inline v3 operator-(v3 a, float b)
{
  return { a.x - b, a.y - b, a.z - b };
}

inline v3 operator*(v3 a, v3 b)
{
  return { a.x * b.x, a.y * b.y, a.z * b.z };
}
inline v3 operator/(v3 a, v3 b)
{
  return { a.x / b.x, a.y / b.y, a.z / b.z };
}
inline v3 operator+(v3 a, v3 b)
{
  return { a.x + b.x, a.y + b.y, a.z + b.z };
}
inline v3 operator-(v3 a, v3 b)
{
  return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline v3 v3::Normalize()
{
  if (*this == 0.0f) return {};
  float rsqrtMagtitude = Q_rsqrt(Sqr(this->x) + Sqr(this->y) + Sqr(this->z));
  return{ this->x * rsqrtMagtitude, this->y * rsqrtMagtitude , this->z * rsqrtMagtitude };
}

inline v3 V3(float x, float y, float z)
{
  return { x, y, z };
}
inline v3 V3(v2 xy, float z)
{
  return { xy.x, xy.y, z };
}

inline v4 v4::operator*=(v4 a)
{
  return *this = (*this * a);
}
inline v4 v4::operator/=(v4 a)
{
  return *this = (*this / a);
}
inline v4 v4::operator+=(v4 a)
{
  return *this = (*this + a);
}
inline v4 v4::operator-=(v4 a)
{
  return *this = (*this - a);
}
inline bool v4::operator==(v4 a)
{
  return (this->x == a.x && this->y == a.y && this->z == a.z && this->w == a.w);
}
inline bool v4::operator!=(v4 a)
{
  return !(*this == a);
}
inline bool v4::operator<=(v4 a)
{
  return (this->x <= a.x && this->y <= a.y && this->z <= a.z && this->w <= a.w);
}
inline bool v4::operator<(v4 a)
{
  return (this->x < a.x&& this->y < a.y&& this->z < a.z&& this->w < a.w);
}
inline bool v4::operator>(v4 a)
{
  return (this->x > a.x && this->y > a.y && this->z > a.z && this->w > a.w);
}

inline v4 v4::operator*=(float a)
{
  return *this = (*this * a);
}
inline v4 v4::operator/=(float a)
{
  return *this = (*this / a);
}
inline v4 v4::operator+=(float a)
{
  return *this = (*this + a);
}
inline v4 v4::operator-=(float a)
{
  return *this = (*this - a);
}
inline bool v4::operator==(float a)
{
  return (this->x == a && this->y == a && this->z == a && this->w == a);
}
inline bool v4::operator!=(float a)
{
  return !(*this == a);
}
inline bool v4::operator<=(float a)
{
  return (this->x <= a && this->y <= a && this->z <= a && this->w <= a);
}
inline bool v4::operator<(float a)
{
  return (this->x < a&& this->y < a&& this->z < a&& this->w < a);
}
inline bool v4::operator>(float a)
{
  return (this->x > a && this->y > a && this->z > a && this->w > a);
}
inline v4 v4::operator-()
{
  return { -this->x, -this->y, -this->z, -this->w };
}

inline v4 operator*(v4 a, float b)
{
  return { a.x * b, a.y * b, a.z * b, a.w * b };
}
inline v4 operator*(float a, v4 b)
{
  return b * a;
}
inline v4 operator/(v4 a, float b)
{
  return { a.x / b, a.y / b, a.z / b, a.w / b };
}
inline v4 operator+(v4 a, float b)
{
  return { a.x + b, a.y + b, a.z + b, a.w + b };
}
inline v4 operator-(v4 a, float b)
{
  return { a.x - b, a.y - b, a.z - b, a.w - b };
}

inline v4 operator*(v4 a, v4 b)
{
  return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}
inline v4 operator/(v4 a, v4 b)
{
  return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}
inline v4 operator+(v4 a, v4 b)
{
  return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}
inline v4 operator-(v4 a, v4 b)
{
  return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

inline v4 v4::Normalize()
{
  if (*this == 0.0f) return {};
  float rsqrtMagtitude = Q_rsqrt(Sqr(this->x) + Sqr(this->y) + Sqr(this->z) + Sqr(this->w));
  return{ this->x * rsqrtMagtitude, this->y * rsqrtMagtitude, this->z * rsqrtMagtitude, this->w * rsqrtMagtitude };
}

inline v4 V4(float x, float y, float z, float w)
{
  return { x, y, z, w };
}
inline v4 V4(v3 xyz, float w)
{
  return { xyz.x, xyz.y, xyz.z, w };
}

u32 v4::ToU32()
{
  u32 red = this->r < 0.0f ? 0 : RoundToU32(this->r * 255.0f);
  u32 green = this->g < 0.0f ? 0 : RoundToU32(this->g * 255.0f);
  u32 blue = this->b < 0.0f ? 0 : RoundToU32(this->b * 255.0f);
  u32 alpha = this->a < 0.0f ? 0 : RoundToU32(this->a * 255.0f);

  return (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
}

inline v4 Lerp(v4 a, v4 b, float t)
{
  return (1 - t) * a + t * b;
}
#endif