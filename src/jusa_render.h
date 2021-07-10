#ifndef JUSA_RENDER_H
#define JUSA_RENDER_H

#include "jusa_types.h"
#include "jusa_utils.h"
#include "jusa_math.h"

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

inline u32 LinearBlend(u32 source, u32 dest);

#endif