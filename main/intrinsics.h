#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <intrin.h>
#include "types.h"

//TODO: moer compiler!
#ifdef COMPILER_MSVC
#undef COMPILER_LLVM
#elif defined(COMPILER_LLVM) 
#undef COMPILER_MSVC
#elif defined(_MSC_VER) 
#define COMPILER_MSVC
#endif

struct bit_scan_result
{
    bool found;
    u32 index;
};

inline bit_scan_result FindLowestSetBit(u32 value)
{
    bit_scan_result result;
#ifdef COMPILER_MSVC 
    result.found = _BitScanForward((unsigned long *)&result.index, value); 
#else
    for(result.index = 0, result.found = false; result.index < 32 ; ++result.index)
    {
        if((value >> result.index) & 1)
        {
            result.found = true;
            break;
        }
    }
#endif
    return result;
}


u32 RotateLeft(u32 value, i32 shift)
{
  return _rotl(value, shift);
}
u32 RotateRight(u32 value, i32 shift)
{
  return _rotr(value, shift);
}

#endif