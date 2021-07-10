#include "jusa_render.h"

inline u32 LinearBlend(u32 source, u32 dest)
{
  //NOTE: alpha blending!
  float t = (float)(dest >> 24) / 255.0f;

  u32 dR = ((dest >> 16) & 0xFF);
  u32 dG = ((dest >> 8) & 0xFF);
  u32 dB = ((dest >> 0) & 0xFF);

  u32 sR = ((source >> 16) & 0xFF);
  u32 sG = ((source >> 8) & 0xFF);
  u32 sB = ((source >> 0) & 0xFF);

  return ((u32)((i32)((float)(dR - sR) * t) + sR) << 16) |
    ((u32)((i32)((float)(dG - sG) * t) + sG) << 8) |
    ((u32)((i32)((float)(dB - sB) * t) + sB) << 0);

}
