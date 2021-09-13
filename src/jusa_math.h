#ifndef JUSA_MATH_H
#define JUSA_MATH_H

#include "jusa_types.h"

#define PI 3.141592653589793f

inline u32 RoundToU32(float value);
inline float Floor(float value);
inline float Ceil(float value);
inline float Round(float value);

inline float Abs(float value);
inline float Q_rsqrt(float value);
inline float Min(float a, float b);
inline float Max(float a, float b);
inline float Sqr(float x);
inline float Sin(float value);
inline float Cos(float value);
inline float Tan(float value);
inline float Atan2(float a, float b);
inline float Atan(float value);
inline float Lerp(float a, float b, float t);

union v2
{
  struct
  {
    float x, y;
  };
  float e[2];

  inline v2 Normalize();

  inline v2 operator*=(v2 a);
  inline v2 operator/=(v2 a);
  inline v2 operator+=(v2 a);
  inline v2 operator-=(v2 a);
  inline bool operator==(v2 a);
  inline bool operator!=(v2 a);
  inline bool operator<=(v2 a);
  inline bool operator<(v2 a);
  inline bool operator>(v2 a);
  inline v2 operator*=(float a);
  inline v2 operator/=(float a);
  inline v2 operator+=(float a);
  inline v2 operator-=(float a);
  inline v2 operator-();
  inline bool operator!=(float a);
  inline bool operator==(float a);
  inline bool operator<=(float a);
  inline bool operator<(float a);
  inline bool operator>(float a);
};

inline v2 Round(v2 value);
inline float Inner(v2 a, v2 b);
inline v2 V2(float x, float y);
inline v2 operator*(v2 a, v2 b);
inline v2 operator/(v2 a, v2 b);
inline v2 operator+(v2 a, v2 b);
inline v2 operator-(v2 a, v2 b);
inline v2 operator*(v2 a, float b);
inline v2 operator*(float a, v2 b);
inline v2 operator/(v2 a, float b);
inline v2 operator+(v2 a, float b);
inline v2 operator-(v2 a, float b);
inline v2 Abs(v2 v);
inline v2 Perp(v2 v);

union v3
{
  struct
  {
    float x, y, z;
  };
  struct
  {
    v2 xy;
    float z;
  };
  float e[3];

  inline v3 Normalize();

  inline v3 operator*=(v3 a);
  inline v3 operator/=(v3 a);
  inline v3 operator+=(v3 a);
  inline v3 operator-=(v3 a);
  inline bool operator==(v3 a);
  inline bool operator!=(v3 a);
  inline bool operator<=(v3 a);
  inline bool operator<(v3 a);
  inline bool operator>(v3 a);
  inline v3 operator*=(float a);
  inline v3 operator/=(float a);
  inline v3 operator+=(float a);
  inline v3 operator-=(float a);
  inline v3 operator-();
  inline bool operator!=(float a);
  inline bool operator==(float a);
  inline bool operator<=(float a);
  inline bool operator<(float a);
  inline bool operator>(float a);
};

inline v3 Round(v3 value);
inline v3 V3(float x, float y, float z);
inline v3 V3(v2 xy, float z);
inline v3 operator*(v3 a, v3 b);
inline v3 operator/(v3 a, v3 b);
inline v3 operator+(v3 a, v3 b);
inline v3 operator-(v3 a, v3 b);
inline v3 operator*(v3 a, float b);
inline v3 operator*(float a, v3 b);
inline v3 operator/(v3 a, float b);
inline v3 operator+(v3 a, float b);
inline v3 operator-(v3 a, float b);
inline v3 Abs(v3 v);

union v4
{
  struct
  {
    float x, y, z, w;
  };
  float e[4];

  u32 ToU32();

  inline v4 Normalize();

  inline v4 operator*=(v4 a);
  inline v4 operator/=(v4 a);
  inline v4 operator+=(v4 a);
  inline v4 operator-=(v4 a);
  inline bool operator==(v4 a);
  inline bool operator!=(v4 a);
  inline bool operator<=(v4 a);
  inline bool operator<(v4 a);
  inline bool operator>(v4 a);
  inline v4 operator*=(float a);
  inline v4 operator/=(float a);
  inline v4 operator+=(float a);
  inline v4 operator-=(float a);
  inline v4 operator-();
  inline bool operator!=(float a);
  inline bool operator==(float a);
  inline bool operator<=(float a);
  inline bool operator<(float a);
  inline bool operator>(float a);
};

inline v4 Round(v4 value);
inline v4 V4(float x, float y, float z, float w);
inline v4 V4(v3 xyz, float w);
inline v4 operator*(v4 a, v4 b);
inline v4 operator/(v4 a, v4 b);
inline v4 operator+(v4 a, v4 b);
inline v4 operator-(v4 a, v4 b);
inline v4 operator*(v4 a, float b);
inline v4 operator*(float a, v4 b);
inline v4 operator/(v4 a, float b);
inline v4 operator+(v4 a, float b);
inline v4 operator-(v4 a, float b);
inline v4 Abs(v4 v);

inline v4 Lerp(v4 a, v4 b, float t);

union rec
{
  struct
  {
    v2 pos;
    v2 size;
  };
  struct
  {
    float x, y, width, height;
  };

  inline v2 rec::GetMinBound()
  {
    //TODO: Test
    return { this->pos.x - (this->width / 2.0f), this->pos.y - (this->height / 2.0f) };
  }

  inline v2 rec::GetMaxBound()
  {
    //TODO: Test
    return { this->pos.x + (this->width / 2.0f), this->pos.y + (this->height / 2.0f) };
  }
};

struct box
{
  v3 pos;
  v3 size;
};
#endif