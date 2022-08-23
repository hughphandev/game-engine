#ifndef JUSA_PHYSICS_H
#define JUSA_PHYSICS_H
#include "jusa_math.h"
#include "jusa_utils.h"

struct ray_cast_hit
{
  bool isHit;
  float t;
  v2 cPoint;
  v2 cNormal;
};

#endif