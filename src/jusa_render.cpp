#include "jusa_render.h"
#include "jusa_math.cpp"

v4 SRGB1ToLinear1(v4 col)
{
  //TODO: Current gamma is 2
  v4 result;
  result.r = col.r * col.r;
  result.g = col.g * col.g;
  result.b = col.b * col.b;
  result.a = col.a;

  return result;
}

v4 SRGB255ToLinear1(v4 col)
{
  return SRGB1ToLinear1(col / 255.0f);
}

v4 Linear1ToSRGB1(v4 col)
{
  //TODO: Current gamma is 2
  v4 result;
  result.r = Sqrt(col.r);
  result.g = Sqrt(col.g);
  result.b = Sqrt(col.b);
  result.a = col.a;

  return result;
}

v4 Linear1ToSRGB255(v4 col)
{
  return Linear1ToSRGB1(col) * 255.0f;
}

v4 UnpackToRGBA255(u32 col)
{
  v4 result = V4((float)((col >> 16) & 0xFF), (float)((col >> 8) & 0xFF), (float)((col >> 0) & 0xFF), (float)((col >> 24) & 0xFF));
  return result;
};

struct bilinear_sample
{
  u32 a, b, c, d;
};

bilinear_sample BilinearSample(loaded_bitmap* bitmap, i32 x, i32 y)
{
  i32 maxX = (x == (bitmap->width - 1)) ? x : x + 1;
  i32 maxY = (y == (bitmap->height - 1)) ? y : y + 1;
  bilinear_sample result;
  result.a = bitmap->pixel[y * bitmap->width + x];
  result.b = bitmap->pixel[y * bitmap->width + maxX];
  result.c = bitmap->pixel[maxY * bitmap->width + x];
  result.d = bitmap->pixel[maxY * bitmap->width + maxX];
  return result;
}

v4 SRGBBilinearBlend(bilinear_sample sample, float tX, float tY)
{
  //NOTE: output in linear color scale 0-1
  v4 texelSampleA = UnpackToRGBA255(sample.a);
  v4 texelSampleB = UnpackToRGBA255(sample.b);
  v4 texelSampleC = UnpackToRGBA255(sample.c);
  v4 texelSampleD = UnpackToRGBA255(sample.d);

  texelSampleA = SRGB255ToLinear1(texelSampleA);
  texelSampleB = SRGB255ToLinear1(texelSampleB);
  texelSampleC = SRGB255ToLinear1(texelSampleC);
  texelSampleD = SRGB255ToLinear1(texelSampleD);

  v4 texel = Lerp(Lerp(texelSampleA, texelSampleB, tX), Lerp(texelSampleC, texelSampleD, tX), tY);
  return texel;
}

inline v4 AlphaBlend(v4 color, v4 baseColor)
{
  float invSA = (1 - color.a);
  v4 result = invSA * baseColor + color;
  return result;
}

inline u32 AlphaBlend(u32 color, u32 baseColor)
{
  v4 d = UnpackToRGBA255(baseColor) / 255.0f;
  v4 s = UnpackToRGBA255(color) / 255.0f;

  v4 col = AlphaBlend(s, d) * 255.0f;

  u32 result = ((u32)col.r << 16) | ((u32)col.g << 8) | ((u32)col.b << 0) | ((u32)col.a << 24);
  return result;
}

void DrawLine(loaded_bitmap* drawBuffer, v2 from, v2 to, u32 lineColor)
{
  //NOTE: this will draw line in screen coord
  u32* pixel = (u32*)drawBuffer->pixel;
  if (from.x == to.x)
  {
    int startY = (int)Round(Min(from.y, to.y));
    int endY = (int)Round(Max(from.y, to.y));
    int x = (int)Round(from.x);
    int screenBound = drawBuffer->width * drawBuffer->height;
    for (int i = startY; i < endY; ++i)
    {
      int pixelIndex = i * drawBuffer->width + x;
      if (pixelIndex >= 0 && pixelIndex < screenBound)
      {
        pixel[i * drawBuffer->width + x] = lineColor;
      }
    }
  }
  else
  {
    float a = (to.y - from.y) / (to.x - from.x);
    float b = from.y - (a * from.x);
    int startX = (int)Round(Min(from.x, to.x));
    int endX = (int)Round(Max(from.x, to.x));
    for (int i = startX; i < endX; ++i)
    {
      int y = (int)Round(a * (float)i + b);
      if (i >= 0 && i < drawBuffer->width && y >= 0 && y < drawBuffer->height)
      {
        pixel[y * drawBuffer->width + i] = lineColor;
      }
    }
  }
}

static void DrawRectangle(loaded_bitmap* buffer, v2 min, v2 max, v4 color)
{
  u32* pixel = buffer->pixel;

  min = Clamp(min, V2(0, 0), V2((float)buffer->width, (float)buffer->height));
  max = Clamp(max, V2(0, 0), V2((float)buffer->width, (float)buffer->height));

  for (int y = (int)min.y; y < (int)max.y; ++y)
  {
    for (int x = (int)min.x; x < (int)max.x; ++x)
    {
      int index = y * buffer->width + x;
      pixel[index] = color.ToU32();
    }
  }
}

static void DrawRectangleOutline(loaded_bitmap* buffer, v2 min, v2 max, u32 thickness, v4 color)
{
  u32* pixel = buffer->pixel;

  min = Clamp(min, V2(0, 0), V2((float)buffer->width, (float)buffer->height));
  max = Clamp(max, V2(0, 0), V2((float)buffer->width, (float)buffer->height));

  DrawRectangle(buffer, min, V2(max.x, min.y + thickness), color);
  DrawRectangle(buffer, min, V2(min.x + thickness, max.y), color);
  DrawRectangle(buffer, V2(max.x - thickness, min.y), max, color);
  DrawRectangle(buffer, V2(min.x, max.y - thickness), max, color);
}

static void ChangeSaturation(loaded_bitmap* buffer, float level)
{
  u32* pixel = buffer->pixel;

  for (int y = 0; y < buffer->height; ++y)
  {
    for (int x = 0; x < buffer->width; ++x)
    {
      int index = y * buffer->width + x;

      v4 color = UnpackToRGBA255(pixel[index]);
      color = SRGB255ToLinear1(color);

      float avg = (color.r + color.g + color.b) / 3.0f;
      color.rgb = Lerp(V3(avg, avg, avg), color.rgb, level);

      color = Linear1ToSRGB1(color);
      pixel[index] = color.ToU32();
    }
  }
}

void ClearBuffer(loaded_bitmap* drawBuffer, u32 c)
{
  u32* pixel = drawBuffer->pixel;
  for (int y = 0; y < drawBuffer->height; ++y)
  {
    for (int x = 0; x < drawBuffer->width; ++x)
    {
      *pixel++ = c;
    }
  }
}

loaded_bitmap MakeEmptyBitmap(memory_arena* arena, u32 width, u32 height, bool clearToZero = true)
{
  loaded_bitmap result = {};
  result.pixel = PUSH_ARRAY(arena, u32, width * height);
  if (result.pixel)
  {
    result.width = width;
    result.height = height;
    if (clearToZero)
    {
      ZeroSize(result.pixel, width * height * BITMAP_PIXEL_SIZE);
    }
  }
  return result;
}

void MakeSphereDiffuseMap(loaded_bitmap* bitmap, float cX = 1.0f, float cY = 1.0f)
{
  float invWidth = 1.0f / (bitmap->width - 1.0f);
  float invHeight = 1.0f / (bitmap->height - 1.0f);

  for (int y = 0; y < bitmap->height; ++y)
  {
    for (int x = 0; x < bitmap->width; ++x)
    {
      v2 bitmapUV = V2(x * invWidth, y * invHeight);
      v2 N = V2(cX, cY) * (2.0f * bitmapUV - 1.0f);

      float alpha = 0.0f;
      float zSqr = 1.0f - Sqr(N.x) - Sqr(N.y);
      if (zSqr >= 0.0f)
      {
        alpha = 1.0f;
      }

      v4 col = V4(alpha * V3(0.0f, 0.0f, 0.0f), alpha);

      u32 index = y * bitmap->width + x;
      bitmap->pixel[index] = col.ToU32();
    }
  }
}

void MakeSphereNormalMap(loaded_bitmap* bitmap, float roughness, float cX = 1.0f, float cY = 1.0f)
{
  float invWidth = 1.0f / (bitmap->width - 1.0f);
  float invHeight = 1.0f / (bitmap->height - 1.0f);

  for (int y = 0; y < bitmap->height; ++y)
  {
    for (int x = 0; x < bitmap->width; ++x)
    {
      v2 bitmapUV = V2(x * invWidth, y * invHeight);
      v2 N = V2(cX, cY) * (2.0f * bitmapUV - 1.0f);

      float rsqrt2 = 0.707106781187f;
      v3 normal = V3(0.0f, rsqrt2, rsqrt2);
      float zSqr = 1.0f - Sqr(N.x) - Sqr(N.y);
      if (zSqr >= 0.0f)
      {
        normal = V3(N, Sqrt(zSqr));
      }

      v4 col = V4(0.5f * (normal + 1.0f), roughness);

      u32 index = y * bitmap->width + x;
      bitmap->pixel[index] = col.ToU32();
    }
  }
}

mat4 GetPerspectiveMatrix(camera* cam)
{
  float a = cam->size.x / cam->size.y;
  float s = 1 / Tan(0.5f * cam->rFov);
  float deltaZ = cam->zFar - cam->zNear;

  mat4 result = {};
  result.r0 = V4(s, 0, 0, 0);
  result.r1 = V4(0, s * a, 0, 0);
  result.r2 = V4(0, 0, -cam->zFar / deltaZ, -1);
  result.r3 = V4(0, 0, -(cam->zFar * cam->zNear) / deltaZ, 0);

  return result;
}

mat4 GetTranslateMatrix(v3 pos)
{
  mat4 result = {};
  result.r0 = V4(1, 0, 0, -pos.x);
  result.r1 = V4(0, 1, 0, -pos.y);
  result.r2 = V4(0, 0, 1, -pos.z);
  result.r3 = V4(0, 0, 0, 1);

  return result;
}

mat4 GetScaleMatrix(v3 scale)
{
  mat4 result = {};
  result.r0 = V4(scale.x, 0, 0, 0);
  result.r1 = V4(0, scale.y, 0, 0);
  result.r2 = V4(0, 0, scale.z, 0);
  result.r3 = V4(0, 0, 0, 1);

  return result;
}

mat4 GetLookAtMatrix(v3 pos, v3 target, v3 up)
{
  v3 camFront = Normalize(pos - target);
  v3 camRight = Normalize(Cross(up, camFront));
  v3 camUp = Normalize(Cross(camFront, camRight));

  mat4 result = {};
  result.r0 = V4(camRight.x, camRight.y, camRight.z, 0);
  result.r1 = V4(camUp.x, camUp.y, camUp.z, 0);
  result.r2 = V4(camFront.x, camFront.y, camFront.z, 0);
  result.r3 = V4(0, 0, 0, 1);

  return result;
}

inline v4 WorldPointToScreen(camera* cam, v3 point)
{
  mat4 proj = GetPerspectiveMatrix(cam);
  mat4 trans = GetTranslateMatrix(cam->pos);
  mat4 view = GetLookAtMatrix(cam->pos, cam->pos + cam->dir, V3(0, 1, 0));

  //NOTE: transform from world coordinate to NDC
  v4 homoPoint = proj * view * trans * V4(point, 1);
  homoPoint.xyz /= homoPoint.w;

  //NOTE: transform from NDC to screen coordinate
  homoPoint.xy = cam->size * (0.5f * (homoPoint.xy + 1.0f));
  return homoPoint;
}

inline v2 ScreenPointToWorld(camera* cam, v2 point)
{
  //TODO: Test
  v2 relPos = point - cam->offSet.xy;
  v2 worldCoord = { relPos.x / (float)cam->meterToPixel, relPos.y / (float)cam->meterToPixel };
  return worldCoord + cam->pos.xy;
}

v3 SampleEnvMap(v2 screenSpaceUV, v3 sampleDirection, float roughness, environment_map* map, float distanceFromMapInZ)
{
  //NOTE: screenSpaceUV is normalized
  u32 LODIndex = (u32)(roughness * (float)(ARRAY_COUNT(map->LOD) - 1) + 0.5f);
  ASSERT(LODIndex < ARRAY_COUNT(map->LOD));

  loaded_bitmap* LOD = &map->LOD[LODIndex];

  float UVsPerMeter = 0.1f; //should be different from x and y
  float c = (UVsPerMeter * distanceFromMapInZ) / sampleDirection.y;
  v2 offset = c * V2(sampleDirection.x, sampleDirection.z);
  v2 UV = offset + screenSpaceUV;

  UV.x = Clamp01(UV.x);
  UV.y = Clamp01(UV.y);

  //TODO: actually calculate x and y
  float xPixel = (UV.x * (float)(LOD->width - 2));
  float yPixel = (UV.y * (float)(LOD->height - 2));

  ASSERT(xPixel >= 0.0f);
  ASSERT(yPixel >= 0.0f);

  i32 xFloor = (i32)(xPixel);
  i32 yFloor = (i32)(yPixel);

  float tX = xPixel - xFloor;
  float tY = yPixel - yFloor;

  bilinear_sample sample = BilinearSample(LOD, xFloor, yFloor);
  v3 result = SRGBBilinearBlend(sample, tX, tY).xyz;

#if 0
  //NOTE: turn on to see where it's reflected
  LOD->pixel[yFloor * LOD->width + xFloor] = 0xFFFFFFFF;
#endif
  return result;
}

v3 Hadamard(v3 texel, v3 col)
{
  return texel * col;
}

v4 Hadamard(v4 texel, v4 col)
{
  return texel * col;
}

v4 BiasNormal(v4 normal)
{
  //NOTE: map normal from 0 -> 1 to -1 -> 1
  return V4((2.0f * normal.xyz) - 1.0f, normal.w);
}

void DrawScreenBitmap(loaded_bitmap* drawBuffer, loaded_bitmap* bitmap, scr_rect bound)
{
  v2 scrMax = V2(drawBuffer->width - 1, drawBuffer->height - 1);
  v2 bitmapMax = V2(bitmap->width - 1, bitmap->height - 1);

  float startY = Ceil(bound.p0.scr.y);
  float endY = Ceil(bound.p1.scr.y);

  v2 scrDeltaL = bound.p1.scr - bound.p0.scr;
  v2 scrDeltaR = bound.p3.scr - bound.p2.scr;

  v3 uvDeltaL = bound.p1.uv - bound.p0.uv;
  v3 uvDeltaR = bound.p3.uv - bound.p2.uv;

  float tStepYL = (1.0f / scrDeltaL.y);
  float tStepYR = (1.0f / scrDeltaR.y);
  v3 uvStepYL = uvDeltaL * tStepYL;
  v3 uvStepYR = uvDeltaR * tStepYR;

  v3 uvPreStepYL = uvDeltaL * (startY - bound.p0.scr.y) * tStepYL;
  v3 uvPreStepYR = uvDeltaR * (startY - bound.p2.scr.y) * tStepYR;

  v3 uvStartX = bound.p0.uv + uvPreStepYL;
  v3 uvEndX = bound.p2.uv + uvPreStepYR;

  for (float y = startY; y < endY; ++y, uvStartX += uvStepYL, uvEndX += uvStepYR)
  {
    float startX = Lerp(bound.p0.scr.x, bound.p1.scr.x, tStepYL * (y - bound.p0.scr.y));

    float endX = Lerp(bound.p2.scr.x, bound.p3.scr.x, tStepYR * (y - bound.p2.scr.y));
    for (float x = startX; x < endX; ++x)
    {
      // mapping
      v3 uvPos = Lerp(uvStartX, uvEndX, (x - startX) / (endX - startX));
      v2 scrPos = Clamp(V2(x, y), V2(0, 0), scrMax);

      v2 bmPos = (uvPos.xy / uvPos.z) * bitmapMax;

      i32 xFloor = (i32)(bmPos.x);
      i32 yFloor = (i32)(bmPos.y);

      float tX = bmPos.x - xFloor;
      float tY = bmPos.y - yFloor;

      bilinear_sample texelSample = BilinearSample(bitmap, xFloor, yFloor);
      v4 texel = SRGBBilinearBlend(texelSample, tX, tY);


      // texel = Hadamard(texel, color);
      texel.r = Clamp01(texel.r);
      texel.g = Clamp01(texel.g);
      texel.b = Clamp01(texel.b);

      i32 scrIndex = (i32)(scrPos.y * drawBuffer->width + scrPos.x);
      u32* screenPixel = &drawBuffer->pixel[scrIndex];
      v4 dest = UnpackToRGBA255(*screenPixel);

      dest = SRGB255ToLinear1(dest);

      v4 linearCol = AlphaBlend(texel, dest);
      *screenPixel = Linear1ToSRGB1(linearCol).ToU32();
    }
  }
}


void DrawTriangle(loaded_bitmap* drawBuffer, camera* cam, triangle2 tri, v4 color, loaded_bitmap* bitmap, loaded_bitmap* normalMap, environment_map* top, environment_map* middle, environment_map* bottom)
{
  v3 uv[3];
  for (i32 i = 0; i < 3; ++i)
  {
    float zInv = 1.0f / tri.scrP[i].w;
    uv[i] = V3(tri.uvP[i] * zInv, zInv);
  }

  i32 count = ARRAY_COUNT(tri.scrP);
  for (i32 i = 0; i < count; ++i)
  {
    for (i32 j = i + 1; j < count; ++j)
    {
      if (tri.scrP[i].y > tri.scrP[j].y)
      {
        Swap(&tri.scrP[i], &tri.scrP[j]);
        Swap(&uv[i], &uv[j]);
      }
    }
  }

  float tScr = (tri.scrP[1].y - tri.scrP[0].y) / (tri.scrP[2].y - tri.scrP[0].y);

  v4 scrMiddle = Lerp(tri.scrP[0], tri.scrP[2], tScr);
  v3 uvMiddle = Lerp(uv[0], uv[2], tScr);

  v4 scrMiddleL;
  v3 uvMiddleL;

  v4 scrMiddleR;
  v3 uvMiddleR;

  if (scrMiddle.x < tri.scrP[1].x)
  {
    scrMiddleL = scrMiddle;
    scrMiddleR = tri.scrP[1];

    uvMiddleL = uvMiddle;
    uvMiddleR = uv[1];
  }
  else
  {
    scrMiddleL = tri.scrP[1];
    scrMiddleR = scrMiddle;

    uvMiddleL = uv[1];
    uvMiddleR = uvMiddle;
  }

    scr_rect bound1;
    bound1.p0 = { tri.scrP[0].xy, uv[0] };
    bound1.p1 = { scrMiddleL.xy, uvMiddleL };
    bound1.p2 = { tri.scrP[0].xy, uv[0] };
    bound1.p3 = { scrMiddleR.xy, uvMiddleR };
    DrawScreenBitmap(drawBuffer, bitmap, bound1);

    scr_rect bound2;
    bound2.p0 = { scrMiddleL.xy, uvMiddleL };
    bound2.p1 = { tri.scrP[2].xy, uv[2] };
    bound2.p2 = { scrMiddleR.xy, uvMiddleR };
    bound2.p3 = { tri.scrP[2].xy, uv[2] };
    DrawScreenBitmap(drawBuffer, bitmap, bound2);
}

void DrawRectSlowly(loaded_bitmap* drawBuffer, camera* cam, v2 origin, v2 xAxis, v2 yAxis, v4 color, loaded_bitmap* bitmap, loaded_bitmap* normalMap, environment_map* top, environment_map* middle, environment_map* bottom)
{
  mat4 trans = {};
  trans.r0 = Normalize(V4(yAxis.y, -yAxis.x, 0, 0));
  trans.r1 = Normalize(V4(-xAxis.y, xAxis.x, 0, 0));

  v2 screenMax = V2((float)drawBuffer->width - 1, (float)drawBuffer->height - 1);

  float originZ = 0.0f;
  float originY = origin.y + 0.5f * xAxis.y + 0.5f * yAxis.y;
  float fixedCastY = originY / screenMax.y;

  float xAxisLength = Length(xAxis);
  float yAxisLength = Length(yAxis);
  //NOTE: nZScale can be a parameter to control scaling in z
  float nZScale = 0.5f * (xAxisLength + yAxisLength);

  v2 nXAxis = (yAxisLength / xAxisLength) * xAxis;
  v2 nYAxis = (xAxisLength / yAxisLength) * yAxis;

  float invSqrlengthX = 1.0f / (Sqr(xAxis.x) + Sqr(xAxis.y));
  float invSqrlengthY = 1.0f / (Sqr(yAxis.x) + Sqr(yAxis.y));

  u32* pixel = (u32*)drawBuffer->pixel;

  float xList[] = { origin.x, origin.x + xAxis.x, origin.x + yAxis.x, origin.x + xAxis.x + yAxis.x };
  int xCount = ARRAY_COUNT(xList);

  float yList[] = { origin.y, origin.y + xAxis.y, origin.y + yAxis.y, origin.y + xAxis.y + yAxis.y };
  int yCount = ARRAY_COUNT(yList);

  int xMin = (int)Min(xList, xCount);
  int xMax = (int)Max(xList, xCount);
  int yMin = (int)Min(yList, yCount);
  int yMax = (int)Max(yList, yCount);

  xMin = Clamp(xMin, 0, drawBuffer->width);
  xMax = Clamp(xMax, 0, drawBuffer->width);
  yMin = Clamp(yMin, 0, drawBuffer->height);
  yMax = Clamp(yMax, 0, drawBuffer->height);

  for (int x = xMin; x < xMax; x++)
  {
    for (int y = yMin; y < yMax; y++)
    {
      ASSERT(x >= 0 && x < drawBuffer->width);
      ASSERT(y >= 0 && y < drawBuffer->height);
      v2 screenP = V2((float)x, (float)y);

      v2 d = screenP - origin;

      float edge1 = Inner(-Perp(xAxis), d);
      float edge2 = Inner(Perp(yAxis), d - yAxis);
      float edge3 = Inner(-Perp(yAxis), d - xAxis);
      float edge4 = Inner(Perp(xAxis), d - xAxis - yAxis);

      if (edge1 < 0 && edge2 < 0 && edge3 < 0 && edge4 < 0)
      {
        v2 screenSpaceUV = V2(screenP.x / screenMax.x, fixedCastY);

        float zDiff = ((float)y - originY) / cam->meterToPixel;

        // float u = Inner(d, xAxis) * invSqrlengthX;
        // float v = Inner(d, yAxis) * invSqrlengthy;

        v4 dTemp = trans * V4(d.x, d.y, 0, 0);
        v2 pUV = V2(dTemp.x, dTemp.y);

        float u = pUV.x / xAxisLength;
        float v = pUV.y / yAxisLength;

        ASSERT(bitmap->width > 2);
        ASSERT(bitmap->height > 2);

        float xPixel = (u * (float)(bitmap->width - 2));
        float yPixel = (v * (float)(bitmap->height - 2));

        i32 xFloor = (i32)(xPixel);
        i32 yFloor = (i32)(yPixel);

        float tX = xPixel - xFloor;
        float tY = yPixel - yFloor;

        bilinear_sample texelSample = BilinearSample(bitmap, xFloor, yFloor);
        v4 texel = SRGBBilinearBlend(texelSample, tX, tY);

        if (normalMap)
        {
          bilinear_sample normalSample = BilinearSample(normalMap, xFloor, yFloor);

          v4 normalSampleA = UnpackToRGBA255(normalSample.a);
          v4 normalSampleB = UnpackToRGBA255(normalSample.b);
          v4 normalSampleC = UnpackToRGBA255(normalSample.c);
          v4 normalSampleD = UnpackToRGBA255(normalSample.d);

          v4 normal = Lerp(Lerp(normalSampleA, normalSampleB, tX), Lerp(normalSampleC, normalSampleD, tX), tY) / 255.0f;
          normal = BiasNormal(normal);

          //NOTE: rotate normal base on x,y axis
          normal.xy = normal.x * nXAxis + normal.y * nYAxis;
          normal.z *= nZScale;
          normal = Normalize(normal);

          //TODO: actually calculate 3d bounce direction

          //NOTE: assume eye vector point in (0, 0, 1) screen space direction
          v3 bounceDirection = 2.0f * normal.z * normal.xyz;
          bounceDirection.z -= 1.0f;

          bounceDirection.z = -bounceDirection.z;

          float tFarMap = 0.0f;
          float pZ = originZ + zDiff;
          float tEnvMap = bounceDirection.y;
          environment_map* farMap = middle;
          if (tEnvMap < -0.5f)
          {
            farMap = bottom;
            tFarMap = -1.0f - (2.0f * tEnvMap);
          }
          else if (tEnvMap > 0.5f)
          {
            farMap = top;
            tFarMap = 2.0f * (tEnvMap - 0.5f);
          }
          else
          {
            //TODO: sample from the middle map
            farMap = 0;
          }

          v3 lightCol = { 0, 0, 0 };
          if (farMap)
          {
            float distanceFromMapInZ = farMap->pZ - pZ;
            v3 farMapCol = SampleEnvMap(screenSpaceUV, bounceDirection, normal.w, farMap, distanceFromMapInZ);
            lightCol = Lerp(lightCol, farMapCol, tFarMap);
          }
          texel.rgb = texel.rgb + texel.a * lightCol;

          texel.rgb *= texel.a;

          if (u > 0.49f && u < 0.51f)
          {
            texel.rgb = V3(0, 0, 0);
          }
        }
        texel = Hadamard(texel, color);
        texel.r = Clamp01(texel.r);
        texel.g = Clamp01(texel.g);
        texel.b = Clamp01(texel.b);

        u32* screenPixel = &pixel[y * drawBuffer->width + x];
        v4 dest = UnpackToRGBA255(*screenPixel);

        dest = SRGB255ToLinear1(dest);

        v4 linearCol = AlphaBlend(texel, dest);
        *screenPixel = Linear1ToSRGB1(linearCol).ToU32();
      }
    }
  }
}

inline void DrawBitmap(loaded_bitmap* drawBuffer, loaded_bitmap* bitmap, v2 minP, v2 maxP)
{
  if (maxP.x < minP.x) return;
  if (maxP.y < minP.y) return;

  if (minP.x >= drawBuffer->width) return;
  if (minP.y >= drawBuffer->height) return;
  if (maxP.x < 0) return;
  if (maxP.y < 0) return;

  v2 size = maxP - minP;

  for (int y = (int)minP.y; y < (int)maxP.y; y++)
  {
    for (int x = (int)minP.x; x < (int)maxP.x; x++)
    {
      if (x < 0 || x >= drawBuffer->width) continue;
      if (y < 0 || y >= drawBuffer->height) continue;

      v2 uv = (V2((float)x, (float)y) - minP) / size;
      v2 p = uv * V2((float)bitmap->width - 1, (float)bitmap->height - 1);

      float tX = p.x - (u32)p.x;
      float tY = p.y - (u32)p.y;

      bilinear_sample sample = BilinearSample(bitmap, (u32)p.x, (u32)p.y);
      v4 col = SRGBBilinearBlend(sample, tX, tY);

      u32* pixel = &drawBuffer->pixel[y * drawBuffer->width + x];
      v4 base = SRGB255ToLinear1(UnpackToRGBA255(*pixel));

      col = AlphaBlend(col, base);
      col = Linear1ToSRGB1(col);

      *pixel = col.ToU32();
    }
  }
}

static void RenderGroupOutput(render_group* renderGroup, loaded_bitmap* drawBuffer)
{

  for (int index = 0; index < renderGroup->pushBuffer.used;)
  {
    render_entry_header* header = (render_entry_header*)((u8*)renderGroup->pushBuffer.base + index);
    index += sizeof(*header);

    switch (header->type)
    {
      case RENDER_TYPE_render_entry_clear:
      {
        render_entry_clear* entry = (render_entry_clear*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        ClearBuffer(drawBuffer, entry->color.ToU32());
      } break;

      case RENDER_TYPE_render_entry_saturation:
      {
        render_entry_saturation* entry = (render_entry_saturation*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        ChangeSaturation(drawBuffer, entry->level);
      } break;

      case RENDER_TYPE_render_entry_rectangle:
      {
        render_entry_rectangle* entry = (render_entry_rectangle*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        DrawRectangle(drawBuffer, entry->minP, entry->maxP, entry->color);
      } break;

      case RENDER_TYPE_render_entry_rectangle_outline:
      {
        render_entry_rectangle_outline* entry = (render_entry_rectangle_outline*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        DrawRectangleOutline(drawBuffer, entry->minP, entry->maxP, entry->thickness, entry->color);
      } break;

      case RENDER_TYPE_render_entry_bitmap:
      {
        render_entry_bitmap* entry = (render_entry_bitmap*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        DrawBitmap(drawBuffer, entry->bitmap, entry->minP, entry->maxP);

      } break;

      case RENDER_TYPE_render_entry_coordinate_system:
      {
        render_entry_coordinate_system* entry = (render_entry_coordinate_system*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        v2 halfSize = { 2, 2 };
        v4 yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
        i32 pointCount = ARRAY_COUNT(entry->point);
        for (i32 i = 0; i < pointCount; ++i)
        {
          DrawRectangle(drawBuffer, entry->point[i].xy - halfSize, entry->point[i].xy + halfSize, yellow);
        }

        triangle2 tri1 = { { entry->point[0], entry->point[1], entry->point[2] }, { V2(0, 0), V2(1, 0), V2(1, 1) } };
        DrawTriangle(drawBuffer, renderGroup->cam, tri1, entry->col, entry->bitmap, entry->normalMap, entry->top, entry->middle, entry->bottom);

        triangle2 tri2 = { { entry->point[0], entry->point[2], entry->point[3] }, { V2(0, 0), V2(1, 1), V2(0, 1) } };
        DrawTriangle(drawBuffer, renderGroup->cam, tri2, entry->col, entry->bitmap, entry->normalMap, entry->top, entry->middle, entry->bottom);

      } break;

      INVALID_DEFAULT_CASE;
    }
  }
}

void* PushRenderElement_(render_group* renderGroup, size_t size, render_entry_type type)
{
  render_entry_header* result = (render_entry_header*)PushSize_(&renderGroup->pushBuffer, size + sizeof(render_entry_header));

  if (result)
  {
    result->type = type;
  }
  else
  {
    INVALID_CODE_PATH;
  }

  return result + 1;
}
#define PUSH_RENDER_ELEMENT(renderGroup, type) (type*)PushRenderElement_(renderGroup, sizeof(type), RENDER_TYPE_##type) 

inline render_entry_clear* Clear(render_group* renderGroup, v4 color)
{
  render_entry_clear* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_clear);
  if (entry)
  {
    entry->color = color;
  }

  return entry;
}

inline render_entry_saturation* Saturation(render_group* renderGroup, float level)
{
  render_entry_saturation* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_saturation);
  if (entry)
  {
    entry->level = level;
  }

  return entry;
}

inline render_entry_bitmap* PushBitmap(render_group* renderGroup, loaded_bitmap* bitmap, v3 pos, v2 size, v4 col = { 1.0f, 1.0f, 1.0f, 1.0f })
{
  render_entry_bitmap* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_bitmap);
  if (entry)
  {
    entry->bitmap = bitmap;
    v3 halfSize = 0.5f * V3(size, 0);

    v2 minP = WorldPointToScreen(renderGroup->cam, pos - halfSize).xy - V2(bitmap->width, bitmap->height) * bitmap->align;
    v2 maxP = WorldPointToScreen(renderGroup->cam, pos + halfSize).xy - V2(bitmap->width, bitmap->height) * bitmap->align;

    entry->minP = minP;
    entry->maxP = maxP;
    entry->color = col;
  }

  return entry;
}

inline render_entry_rectangle* PushRect(render_group* renderGroup, v3 pos, v2 size, v4 col)
{
  render_entry_rectangle* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_rectangle);
  if (entry)
  {
    v3 halfSize = 0.5f * V3(size, 0);
    entry->minP = WorldPointToScreen(renderGroup->cam, pos - halfSize).xy;
    entry->maxP = WorldPointToScreen(renderGroup->cam, pos + halfSize).xy;
    entry->color = col;
  }

  return entry;
}

inline render_entry_rectangle_outline* PushRectOutline(render_group* renderGroup, v3 pos, v2 size, u32 thickness, v4 col)
{
  render_entry_rectangle_outline* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_rectangle_outline);
  if (entry)
  {
    v3 halfSize = 0.5f * V3(size, 0);
    entry->minP = WorldPointToScreen(renderGroup->cam, pos - halfSize).xy;
    entry->maxP = WorldPointToScreen(renderGroup->cam, pos + halfSize).xy;
    entry->color = col;
    entry->thickness = thickness;
  }

  return entry;
}

inline render_entry_coordinate_system* CoordinateSystem(render_group* renderGroup, v3 origin, v3 xAxis, v3 yAxis, v3 zAxis, v4 col, loaded_bitmap* bitmap, loaded_bitmap* normalMap, environment_map* top, environment_map* middle, environment_map* bottom)
{
  render_entry_coordinate_system* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_coordinate_system);
  if (entry)
  {
    entry->point[0] = WorldPointToScreen(renderGroup->cam, origin);
    entry->point[1] = WorldPointToScreen(renderGroup->cam, origin + xAxis);
    entry->point[2] = WorldPointToScreen(renderGroup->cam, origin + yAxis + xAxis);
    entry->point[3] = WorldPointToScreen(renderGroup->cam, origin + yAxis);
    entry->col = col;
    entry->bitmap = bitmap;
    entry->normalMap = normalMap;
    entry->top = top;
    entry->middle = middle;
    entry->bottom = bottom;
  }

  return entry;
}

inline render_group* InitRenderGroup(memory_arena* arena, size_t maxPushBufferSize, camera* cam)
{
  render_group* result = PUSH_TYPE(arena, render_group);
  InitMemoryArena(&result->pushBuffer, maxPushBufferSize, PUSH_SIZE(arena, maxPushBufferSize));
  result->cam = cam;

  return result;
}