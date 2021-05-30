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
  inline v2 operator=(float a);
  inline v2 operator*=(float a);
  inline v2 operator/=(float a);
  inline v2 operator+=(float a);
  inline v2 operator-=(float a);
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

#endif