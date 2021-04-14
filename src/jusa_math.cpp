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

enum solution_type
{
  NONE, ONE, INFINITE
};

struct gauss_j_result
{
  solution_type type;
  float* value;
  u32 order;

  bool operator==(gauss_j_result v)
  {
    bool result = (this->type == v.type && this->order == v.order);
    
    for(u32 i = 0; i < order && result; ++i)
    {
      result = (this->value[i] == v.value[i]);
    }
    return result;
  }
};

gauss_j_result Gauss_J(float* a, i32 height, i32 width)
{
  // init
  gauss_j_result result = {};
  result.order = height;
  result.value = (float*)malloc(result.order * sizeof(float));
  for (u32 i = 0; i < result.order; ++i)
  {
    result.value[i] = 0.0f;
  }

  u32 size = width * height;
  float* m = (float*)_alloca(size * sizeof(float));

  // memcpy
  for (u32 i = 0; i < size; ++i)
  {
    m[i] = a[i];
  }

  // make sure a[i][i] != 0 as much as possible!
  for (int i = 0; i < height; ++i)
  {
    if (m[i * width + i] == 0)
    {
      i32 c = 1;
      while ((i + c) < height && m[(i + c) * width + i] == 0) ++c;

      if ((i + c) < height)
      {
        for (i32 j = i, k = 0; k < width; k++)
          Swap(&m[j * width + k], &m[(j + c) * width + k]);
      }
    }
  }

  //NOTE: every rows reform with every other rows!
  for (i32 i = 0; i < height; i++)
  {
    for (i32 j = 0; j < height; j++) {
      if (i != j) {

        float pro = m[j * width + i] / m[i * width + i];

        for (i32 k = 0; k < width; k++)
          m[j * width + k] = m[j * width + k] - m[i * width + k] * pro;
      }
    }
  }

  // post-check and get result value
  for (i32 i = 0; i < height; ++i)
  {
    i32 rIndex = i * width + width - 1;
    i32 lIndex = i * width + i;
    float res = m[rIndex] / m[lIndex];
    if (isnan(res)) result.type = INFINITE;
    else if (!isfinite(res)) result.type = NONE;
    else if (i == (height - 1))
    {
      result.type = ONE;
      result.value[i] = res;
    }
    else
    {
      result.value[i] = res;
    }
  }
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