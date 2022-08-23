#ifndef PHYSICS_CPP
#define PHYSICS_CPP

#include "physics.h"

bool IsRecOverlapping(rec a, rec b)
{
  //TODO: Test
  bool result = false;
  v2 aMin = a.GetMinBound();
  v2 aMax = a.GetMaxBound();
  v2 bMin = b.GetMinBound();
  v2 bMax = b.GetMaxBound();

  result = (aMin.x < bMax.x) && (aMax.x > bMin.x) && (aMax.y > bMin.y) && (aMin.y < bMax.y);

  return result;
}

ray_cast_hit RayToRec(v2 rayOrigin, v2 rayVec, rec r)
{
  ray_cast_hit result = {};
  if (rayVec == 0.0f) return { false, 0, {}, {} };
  v2 minPos = r.GetMinBound();
  v2 maxPos = r.GetMaxBound();

  v2 tNear = (minPos - rayOrigin) / rayVec;
  v2 tFar = (maxPos - rayOrigin) / rayVec;

  //Note: check if value is a floating point number!
  if (tFar != tFar || tNear != tNear) return { false, 0, {}, {} };

  if (tNear.x > tFar.x) Swap(&tNear.x, &tFar.x);
  if (tNear.y > tFar.y) Swap(&tNear.y, &tFar.y);

  if (tNear.x > tFar.y || tNear.y > tFar.x) return result;

  result.t = Max(tNear.x, tNear.y);
  float tHitFar = Min(tFar.x, tFar.y);

  if (result.t < 0.0f || (result.t) > 1.0f)
  {
    result.isHit = false;
  }
  else
  {
    result.isHit = true;
    result.cPoint = rayOrigin + (rayVec * result.t);

    if (tNear.x > tNear.y)
    {
      if (rayVec.x > 0.0)
      {
        result.cNormal = { -1.0f, 0.0f };
      }
      else
      {
        result.cNormal = { 1.0f, 0.0f };
      }
    }
    else
    {
      if (rayVec.y < 0.0f)
      {
        result.cNormal = { 0.0f, 1.0f };
      }
      else
      {
        result.cNormal = { 0.0f, -1.0f };
      }
    }
  }
  return result;
}

#endif