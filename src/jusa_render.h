#ifndef JUSA_RENDER_H
#define JUSA_RENDER_H

#include "jusa_types.h"
#include "jusa_math.h"
#include "jusa_utils.h"

enum brush_type
{
  BRUSH_FILL,
  BRUSH_WIREFRAME
};

struct color
{
  float r, g, b, a;

  u32 ToU32();
};

u32 color::ToU32()
{
  ASSERT(this->r >= 0.0f);
  ASSERT(this->g >= 0.0f);
  ASSERT(this->b >= 0.0f);
  ASSERT(this->a >= 0.0f);


  u32 red = RoundToU32(this->r * 255.0f);
  u32 green = RoundToU32(this->g * 255.0f);
  u32 blue = RoundToU32(this->b * 255.0f);
  u32 alpha = RoundToU32(this->a * 255.0f);

  return (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
}

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

#endif