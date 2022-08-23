#ifndef JUSA_UTILS_H
#define JUSA_UTILS_H

#include "jusa_types.h"

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#define KILOBYTES(value) ((value) * 1024LL) 
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL) 
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL) 
#define TERABYTES(value) (GIGABYTES(value) * 1024LL) 

#if SLOW
#define ASSERT(Expression) if(!(Expression)) {*(int *)0 = 0;}
inline u32 SafeTruncateUInt64(u64 value);
#else
#define ASSERT(Expression) 
#endif

#define INVALID_CODE_PATH ASSERT(!"Invalid code path!")
#define INVALID_DEFAULT_CASE default:{ASSERT(!"Invalid code path!");} 

inline uint32_t SafeTruncateUInt64(uint64_t value)
{
  // TODO: Define maximum value
  ASSERT(value < 0xFFFFFFFF);
  return (uint32_t)value;
}

//NOTE: non-pointer type only
#define DEFINE_SWAP(T)  void Swap(T* l, T* r){ T temp = *l; *l = *r; *r = temp; }

DEFINE_SWAP(float)
DEFINE_SWAP(i32)

float Min(float* value, int count)
{
  ASSERT(count > 0);

  float result = value[0];
  for (int i = 0; i < count; ++i)
  {
    if (result > value[i])
    {
      result = value[i];
    }
  }
  return result;
}

float Max(float* value, int count)
{
  ASSERT(count > 0);

  float result = value[0];
  for (int i = 0; i < count; ++i)
  {
    if (result < value[i])
    {
      result = value[i];
    }
  }
  return result;
}

#include <memory.h>

void Memcpy(void* dest, void* src, size_t size)
{
  memcpy(dest, src, size);
}

void ZeroSize(void* mem, size_t size)
{
  memset(mem, 0, size);
}

inline i32 StrToI(char* str, char** pStr)
{
  int sign = 1;
  int result = 0;
  while (*str == '-')
  {
    sign *= -1;
    ++str;
  }
  while (*str >= '0' && *str <= '9')
  {
    result = result * 10 + (*str++ - '0');
  }
  return result * sign;
}


#endif