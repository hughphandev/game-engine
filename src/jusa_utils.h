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

inline uint32_t SafeTruncateUInt64(uint64_t value)
{
  // TODO: Define maximum value
  ASSERT(value < 0xFFFFFFFF);
  return (uint32_t)value;
}

void Swap(float* l, float* r)
{
  float temp = *l;
  *l = *r;
  *r = temp;
}

void Swap(i32* l, i32* r)
{
  i32 temp = *l;
  *l = *r;
  *r = temp;
}

#include <memory.h>

void Memcpy(void* dest, void* src, size_t size)
{
  memcpy(dest, src, size);
}


#endif