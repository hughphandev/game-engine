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

inline float Sqrt(float x)
{
  return sqrtf(x);
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

inline float Clamp01(float a)
{
  if (a > 1.0f) return 1.0f;
  if (a < 0.0f) return 0.0f;
  return a;
}

inline float Clamp(float a, float min, float max)
{
  float result = a;
  if (a < min) result = min;
  if (a > max) result = max;
  return result;
}
inline int Clamp(int a, int min, int max)
{
  int result = a;
  if (a < min) result = min;
  if (a > max) result = max;
  return result;
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

inline v2 V2(i32 x, i32 y)
{
  return { (float)x, (float)y };
}

inline v2 Normalize(v2 v)
{
  if (v == 0.0f) return { 0.0f, 0.0f };
  float rsqrtMagtitude = Q_rsqrt(Sqr(v.x) + Sqr(v.y));
  return v * rsqrtMagtitude;
}

inline float Length(v2 v)
{
  return Sqrt(Sqr(v.x) + Sqr(v.y));
}

inline v2 Clamp(v2 a, v2 min, v2 max)
{
  v2 result;
  result.x = Clamp(a.x, min.x, max.x);
  result.y = Clamp(a.y, min.y, max.y);
  return result;
}

inline float Dot(v2 a, v2 b)
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

inline v3 Normalize(v3 v)
{
  if (v == 0.0f) return {};
  float rsqrtMagtitude = Q_rsqrt(Sqr(v.x) + Sqr(v.y) + Sqr(v.z));
  return v * rsqrtMagtitude;
}

inline float Length(v3 v)
{
  return Sqrt(Sqr(v.x) + Sqr(v.y) + Sqr(v.z));
}

inline v3 Round(v3 value)
{
  return V3(Round(value.x), Round(value.y), Round(value.z));
}

inline v3 Cross(v3 a, v3 b)
{
  v3 result;
  result.x = a.y * b.z - a.z * b.y;
  result.y = a.z * b.x - a.x * b.z;
  result.z = a.x * b.y - a.y * b.x;
  return result;
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

inline v4 Normalize(v4 v)
{
  if (v == 0.0f) return {};
  float rsqrtMagtitude = Q_rsqrt(Sqr(v.x) + Sqr(v.y) + Sqr(v.z) + Sqr(v.w));
  return v * rsqrtMagtitude;
}

inline float Length(v4 v)
{
  return Sqrt(Sqr(v.x) + Sqr(v.y) + Sqr(v.z) + Sqr(v.w));
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

DEFINE_LERP(float);
DEFINE_LERP(v2);
DEFINE_LERP(v3);
DEFINE_LERP(v4);
//
// Matrix operation
//

inline mat4 operator*(mat4 a, mat4 b)
{
  mat4 result = {};

  for (i32 i = 0; i < 4; ++i)
  {
    for (i32 j = 0; j < 4; ++j)
    {
      for (i32 k = 0; k < 4; ++k)
      {
        result.e[i][j] += a.e[i][k] * b.e[k][j];
      }
    }
  }
  return result;
}

inline mat4 operator+(mat4 a, mat4 b)
{
  mat4 result;
  float* e = (float*)result.e;
  float* e1 = (float*)a.e;
  float* e2 = (float*)b.e;

  for (int i = 0; i < 16; ++i)
  {
    e[i] = e1[i] + e2[i];
  }
  return result;
}

inline mat4 operator-(mat4 a, mat4 b)
{
  mat4 result;
  float* e = (float*)result.e;
  float* e1 = (float*)a.e;
  float* e2 = (float*)b.e;

  for (int i = 0; i < 16; ++i)
  {
    e[i] = e1[i] - e2[i];
  }
  return result;
}

inline mat4 operator*(mat4 a, float b)
{
  mat4 result;
  float* e = (float*)result.e;
  float* e1 = (float*)a.e;

  for (int i = 0; i < 16; ++i)
  {
    e[i] = e1[i] * b;
  }
  return result;
}

inline mat4 operator*(float a, mat4 b)
{
  mat4 result;
  float* e = (float*)result.e;
  float* e2 = (float*)b.e;

  for (int i = 0; i < 16; ++i)
  {
    e[i] = a * e2[i];
  }
  return result;
}

inline mat4 operator/(mat4 a, float b)
{
  mat4 result;
  float* e = (float*)result.e;
  float* e1 = (float*)a.e;

  for (int i = 0; i < 16; ++i)
  {
    e[i] = e1[i] / b;
  }
  return result;
}

inline mat4 operator+(mat4 a, float b)
{
  mat4 result = a;

  for (int i = 0; i < 4; ++i)
  {
    result.e[i][i] += b;
  }
  return result;
}

inline mat4 operator-(mat4 a, float b)
{
  mat4 result = a;

  for (int i = 0; i < 16; ++i)
  {
    result.e[i][i] -= b;
  }
  return result;
}

inline bool operator==(mat4 a, mat4 b)
{
  bool equal = (a.r0 == b.r0) && (a.r1 == b.r1) && (a.r2 == b.r2) && (a.r3 == b.r3);
  return equal;
}

inline v4 operator*(v4 a, mat4 b)
{
  v4 result = {};

  for (i32 i = 0; i < 4; ++i)
  {
    for (i32 j = 0; j < 4; ++j)
    {
      result.e[i] += b.e[j][i] * a.e[j];
    }
  }
  return result;
}
inline v4 operator*(mat4 a, v4 b)
{
  v4 result = {};

  for (i32 i = 0; i < 4; ++i)
  {
    for (i32 j = 0; j < 4; ++j)
    {
      result.e[i] += a.e[i][j] * b.e[j];
    }
  }
  return result;
}

inline mat4_inverse Inverse(mat4 a)
{
  mat4_inverse result = {};
  float inv[16];
  int i;
  float* m = (float*)a.e;

  inv[0] = m[5] * m[10] * m[15] -
    m[5] * m[11] * m[14] -
    m[9] * m[6] * m[15] +
    m[9] * m[7] * m[14] +
    m[13] * m[6] * m[11] -
    m[13] * m[7] * m[10];

  inv[4] = -m[4] * m[10] * m[15] +
    m[4] * m[11] * m[14] +
    m[8] * m[6] * m[15] -
    m[8] * m[7] * m[14] -
    m[12] * m[6] * m[11] +
    m[12] * m[7] * m[10];

  inv[8] = m[4] * m[9] * m[15] -
    m[4] * m[11] * m[13] -
    m[8] * m[5] * m[15] +
    m[8] * m[7] * m[13] +
    m[12] * m[5] * m[11] -
    m[12] * m[7] * m[9];

  inv[12] = -m[4] * m[9] * m[14] +
    m[4] * m[10] * m[13] +
    m[8] * m[5] * m[14] -
    m[8] * m[6] * m[13] -
    m[12] * m[5] * m[10] +
    m[12] * m[6] * m[9];

  inv[1] = -m[1] * m[10] * m[15] +
    m[1] * m[11] * m[14] +
    m[9] * m[2] * m[15] -
    m[9] * m[3] * m[14] -
    m[13] * m[2] * m[11] +
    m[13] * m[3] * m[10];

  inv[5] = m[0] * m[10] * m[15] -
    m[0] * m[11] * m[14] -
    m[8] * m[2] * m[15] +
    m[8] * m[3] * m[14] +
    m[12] * m[2] * m[11] -
    m[12] * m[3] * m[10];

  inv[9] = -m[0] * m[9] * m[15] +
    m[0] * m[11] * m[13] +
    m[8] * m[1] * m[15] -
    m[8] * m[3] * m[13] -
    m[12] * m[1] * m[11] +
    m[12] * m[3] * m[9];

  inv[13] = m[0] * m[9] * m[14] -
    m[0] * m[10] * m[13] -
    m[8] * m[1] * m[14] +
    m[8] * m[2] * m[13] +
    m[12] * m[1] * m[10] -
    m[12] * m[2] * m[9];

  inv[2] = m[1] * m[6] * m[15] -
    m[1] * m[7] * m[14] -
    m[5] * m[2] * m[15] +
    m[5] * m[3] * m[14] +
    m[13] * m[2] * m[7] -
    m[13] * m[3] * m[6];

  inv[6] = -m[0] * m[6] * m[15] +
    m[0] * m[7] * m[14] +
    m[4] * m[2] * m[15] -
    m[4] * m[3] * m[14] -
    m[12] * m[2] * m[7] +
    m[12] * m[3] * m[6];

  inv[10] = m[0] * m[5] * m[15] -
    m[0] * m[7] * m[13] -
    m[4] * m[1] * m[15] +
    m[4] * m[3] * m[13] +
    m[12] * m[1] * m[7] -
    m[12] * m[3] * m[5];

  inv[14] = -m[0] * m[5] * m[14] +
    m[0] * m[6] * m[13] +
    m[4] * m[1] * m[14] -
    m[4] * m[2] * m[13] -
    m[12] * m[1] * m[6] +
    m[12] * m[2] * m[5];

  inv[3] = -m[1] * m[6] * m[11] +
    m[1] * m[7] * m[10] +
    m[5] * m[2] * m[11] -
    m[5] * m[3] * m[10] -
    m[9] * m[2] * m[7] +
    m[9] * m[3] * m[6];

  inv[7] = m[0] * m[6] * m[11] -
    m[0] * m[7] * m[10] -
    m[4] * m[2] * m[11] +
    m[4] * m[3] * m[10] +
    m[8] * m[2] * m[7] -
    m[8] * m[3] * m[6];

  inv[11] = -m[0] * m[5] * m[11] +
    m[0] * m[7] * m[9] +
    m[4] * m[1] * m[11] -
    m[4] * m[3] * m[9] -
    m[8] * m[1] * m[7] +
    m[8] * m[3] * m[5];

  inv[15] = m[0] * m[5] * m[10] -
    m[0] * m[6] * m[9] -
    m[4] * m[1] * m[10] +
    m[4] * m[2] * m[9] +
    m[8] * m[1] * m[6] -
    m[8] * m[2] * m[5];

  result.det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  if (result.det == 0)
    return result;

  float invDet = 1.0f / result.det;
  result.isExisted = true;

  for (i = 0; i < 16; i++)
    ((float*)result.inv.e)[i] = inv[i] * invDet;

  return result;
}

inline float Det(mat4 a)
{
  float* m[4] = { a.e[0], a.e[1], a.e[2], a.e[3] };
  return
    m[0][3] * m[1][2] * m[2][1] * m[3][0] - m[0][2] * m[1][3] * m[2][1] * m[3][0] -
    m[0][3] * m[1][1] * m[2][2] * m[3][0] + m[0][1] * m[1][3] * m[2][2] * m[3][0] +
    m[0][2] * m[1][1] * m[2][3] * m[3][0] - m[0][1] * m[1][2] * m[2][3] * m[3][0] -
    m[0][3] * m[1][2] * m[2][0] * m[3][1] + m[0][2] * m[1][3] * m[2][0] * m[3][1] +
    m[0][3] * m[1][0] * m[2][2] * m[3][1] - m[0][0] * m[1][3] * m[2][2] * m[3][1] -
    m[0][2] * m[1][0] * m[2][3] * m[3][1] + m[0][0] * m[1][2] * m[2][3] * m[3][1] +
    m[0][3] * m[1][1] * m[2][0] * m[3][2] - m[0][1] * m[1][3] * m[2][0] * m[3][2] -
    m[0][3] * m[1][0] * m[2][1] * m[3][2] + m[0][0] * m[1][3] * m[2][1] * m[3][2] +
    m[0][1] * m[1][0] * m[2][3] * m[3][2] - m[0][0] * m[1][1] * m[2][3] * m[3][2] -
    m[0][2] * m[1][1] * m[2][0] * m[3][3] + m[0][1] * m[1][2] * m[2][0] * m[3][3] +
    m[0][2] * m[1][0] * m[2][1] * m[3][3] - m[0][0] * m[1][2] * m[2][1] * m[3][3] -
    m[0][1] * m[1][0] * m[2][2] * m[3][3] + m[0][0] * m[1][1] * m[2][2] * m[3][3];
}
#endif