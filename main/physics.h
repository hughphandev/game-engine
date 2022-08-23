#ifndef PHYSICS_H
#define PHYSICS_H
#include "math.h"
#include "utils.h"

struct ray_cast_hit
{
  bool isHit;
  float t;
  v2 cPoint;
  v2 cNormal;
};

#endif