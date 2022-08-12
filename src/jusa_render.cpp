#include "jusa_render.h"
#include "jusa_math.cpp"

//TODO: remove
#include "windows.h"
#include "stdio.h"

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

void DrawLine(loaded_bitmap* drawBuffer, v2 from, v2 to, v4 color = V4(1, 1, 1, 1))
{
  //NOTE: this will draw line in screen coord
  int scrMax = (drawBuffer->width * drawBuffer->height) - 1;
  v2 d = to - from;
  float length = Length(d);
  if (d.y == 0)
  {
    for (float x = from.x; x < to.x; ++x)
    {
      int index = (int)(from.y * drawBuffer->width + x);
      if (index >= 0 && index <= scrMax)
      {
        drawBuffer->pixel[index] = color.ToU32();
      }
    }
  }
  else
  {
    float tStep = 1.0f / d.y;
    for (v2 pos = from; pos.y < to.y; pos += tStep * d)
    {
      int index = (int)(pos.y * drawBuffer->width + pos.x);
      if (index >= 0 && index <= scrMax)
      {
        drawBuffer->pixel[index] = color.ToU32();
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
  //TODO: Optimize Performance
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

mat4 GetRotateXMatrix(float value)
{
  return {
    1, 0, 0, 0,
    0, Cos(value), -Sin(value), 0,
    0, Sin(value), Cos(value), 0,
    0, 0, 0, 1,
  };
}

mat4 GetRotateYMatrix(float value)
{
  return {
    Cos(value), 0, Sin(value), 0,
    0, 1, 0, 0,
    -Sin(value), 0, Cos(value), 0,
    0, 0, 0, 1,
  };
}

mat4 GetRotateZMatrix(float value)
{
  return {
    Cos(value), -Sin(value), 0, 0,
    Sin(value), Cos(value), 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
  };
}

v3 Rotate(v3 vec, v3 rotation)
{
  v4 result = GetRotateZMatrix(rotation.z) * GetRotateYMatrix(rotation.y) * GetRotateXMatrix(rotation.x) * V4(vec, 1.0f);
  return result.xyz;
}

mat4 GetPerspectiveMatrix(camera* cam)
{
  float a = cam->size.x / cam->size.y;
  float s = 1 / Tan(0.5f * cam->rFov);
  float deltaZ = cam->zFar - cam->zNear;

  mat4 result = {};
  result.r0 = V4(s, 0, 0, 0);
  result.r1 = V4(0, s * a, 0, 0);
  result.r2 = V4(0, 0, cam->zFar / deltaZ, -(cam->zFar * cam->zNear) / deltaZ);
  result.r3 = V4(0, 0, 1, 0);

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

mat4 GetLookAtMatrix(v3 pos, v3 target, v3 up = V3(0, 1, 0))
{
  v3 dir = Normalize(target - pos);

  v3 camZ = dir;
  v3 camX = Normalize(Cross(up, camZ));
  v3 camY = Normalize(Cross(camZ, camX));

  char buf[256];
  _snprintf_s(buf, sizeof(buf), "dir=%f, %f, %f", dir.x, dir.y, dir.z);
  OutputDebugStringA(buf);


  mat4 result = {};
  result.r0 = V4(camX.x, camX.y, camX.z, -Dot(camX, pos));
  result.r1 = V4(camY.x, camY.y, camY.z, -Dot(camY, pos));
  result.r2 = V4(camZ.x, camZ.y, camZ.z, -Dot(camZ, pos));
  result.r3 = V4(0, 0, 0, 1);

  return result;
}

inline v3 WorldPointToCamera(camera* cam, v3 point)
{
  //NOTE: camera position is (0, 0, 0), camera direction is z axis
  mat4 trans = GetTranslateMatrix(cam->pos);
  mat4 view = GetLookAtMatrix(cam->pos, cam->pos + cam->dir, V3(0, 1, 0));
  v4 result = trans * V4(point, 1);
  result = view * result;
  return result.xyz;
}

inline v3 CameraPointToNDC(camera* cam, v3 point)
{
  //NOTE: transform from camera coordinate to NDC
  mat4 proj = GetPerspectiveMatrix(cam);

  v4 homoPoint = proj * V4(point, 1);
  homoPoint.xyz /= homoPoint.w;

  //TODO: consider scale z from 0 to 1
  return V3(homoPoint.xy, homoPoint.w);
}

inline v2 NDCPointToScreen(camera* cam, v3 point)
{
  return cam->size * (0.5f * (point.xy + 1.0f));
}

inline v2 WorldPointToScreen(camera* cam, v3 point)
{
  v3 ndc = WorldPointToCamera(cam, point);
  v2 result = NDCPointToScreen(cam, ndc);

  return result;
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

vertex GetPointInScreenRow(vertex_line line, float y)
{
  float t = (y - line.origin.pos.y) / line.dir.pos.y;
  vertex result;
  result.pos = line.origin.pos + (t * line.dir.pos);
  result.uv = line.origin.uv + (t * line.dir.uv);
  return result;
}
vertex ShilfPointToNextCol(vertex origin, vertex dir)
{
  //TODO: shift it!
  float newX = Ceil(origin.pos.x);
  float t = (newX - origin.pos.x) / dir.pos.x;
  vertex result = origin;
  result.pos.x = newX;
  result.uv = origin.uv + (t * dir.uv);
  return result;
}

void FillFlatQuadTex(loaded_bitmap* drawBuffer, loaded_bitmap* bitmap, camera* cam, directional_light light, v3 faceNormal, v4 color, vertex_line lineL, vertex_line lineR, float startY, float endY)
{
  v2 scrSize = V2(drawBuffer->width, drawBuffer->height);
  v2 bitmapMax = V2(bitmap->width - 1, bitmap->height - 1);
  v2 scrMax = V2(drawBuffer->width - 1, drawBuffer->height - 1);

#define PS(M, i) *((float*)(&M) + i)
#define PSi(M, i) *((i32*)(&M) + i)
  __m128 zero_x4 = _mm_set_ps1(0.0f);
  __m128 one_x4 = _mm_set_ps1(1.0f);
  __m128 one255_x4 = _mm_set_ps1(255.0f);
  __m128 inv255_x4 = _mm_set_ps1(1.0f / 255.0f);

  __m128 scrWidth_x4 = _mm_set_ps1((float)drawBuffer->width);
  __m128 scrHeight_x4 = _mm_set_ps1((float)drawBuffer->height);

  __m128 bitmapMaxX_x4 = _mm_set_ps1(bitmapMax.x);
  __m128 bitmapMaxY_x4 = _mm_set_ps1(bitmapMax.y);

  float lightMul = Max(Dot(-light.dir, faceNormal), 0.0f);
  __m128 lightMulR_x4 = _mm_set_ps1((lightMul * light.diffuse.x) + light.ambient.x);
  __m128 lightMulG_x4 = _mm_set_ps1((lightMul * light.diffuse.y) + light.ambient.y);
  __m128 lightMulB_x4 = _mm_set_ps1((lightMul * light.diffuse.z) + light.ambient.z);

  for (float y = startY; y < endY; ++y)
  {
    vertex startVert = GetPointInScreenRow(lineL, y);
    vertex endVert = GetPointInScreenRow(lineR, y);

    vertex startVertInCol = ShilfPointToNextCol(startVert, endVert - startVert);

    __m128 startX_x4 = _mm_set_ps1(startVertInCol.pos.x);
    __m128 endX_x4 = _mm_set_ps1(endVert.pos.x);

    __m128 startZ_x4 = _mm_set_ps1(startVertInCol.pos.z);
    __m128 endZ_x4 = _mm_set_ps1(endVert.pos.z);

    __m128 uvStartXX_x4 = _mm_set_ps1(startVertInCol.uv.x);
    __m128 uvStartXY_x4 = _mm_set_ps1(startVertInCol.uv.y);

    __m128 uvEndXX_x4 = _mm_set_ps1(endVert.uv.x);
    __m128 uvEndXY_x4 = _mm_set_ps1(endVert.uv.y);

    float cStartX = Clamp(startVertInCol.pos.x, 0.0f, scrMax.x);
    float cEndX = Clamp(endVert.pos.x, 0.0f, scrMax.x);

    __m128 cEndX_x4 = _mm_set1_ps(cEndX);
    for (float x = cStartX; x < cEndX; x += 4)
    {
      BEGIN_TIMER_BLOCK(PixelFill);

      __m128 x_x4 = _mm_set_ps(x + 3, x + 2, x + 1, x);
      __m128 y_x4 = _mm_set_ps1(y);

      __m128 tUV_x4 = _mm_div_ps(_mm_sub_ps(x_x4, startX_x4), _mm_sub_ps(endX_x4, startX_x4));

      __m128 uvPosX_x4 = _mm_add_ps(uvStartXX_x4, _mm_mul_ps(tUV_x4, _mm_sub_ps(uvEndXX_x4, uvStartXX_x4)));
      __m128 uvPosY_x4 = _mm_add_ps(uvStartXY_x4, _mm_mul_ps(tUV_x4, _mm_sub_ps(uvEndXY_x4, uvStartXY_x4)));
      __m128 posZ = _mm_add_ps(startZ_x4, _mm_mul_ps(tUV_x4, _mm_sub_ps(endZ_x4, startZ_x4)));

      __m128 z_x4 = _mm_div_ps(one_x4, posZ);

      __m128 bmPosX_x4 = _mm_mul_ps(bitmapMaxX_x4, _mm_mul_ps(uvPosX_x4, z_x4));
      __m128 bmPosY_x4 = _mm_mul_ps(bitmapMaxY_x4, _mm_mul_ps(uvPosY_x4, z_x4));

      __m128 xFloor_x4 = _mm_floor_ps(bmPosX_x4);
      __m128 yFloor_x4 = _mm_floor_ps(bmPosY_x4);
      __m128 xCeil_x4 = _mm_min_ps(_mm_add_ps(xFloor_x4, one_x4), bitmapMaxX_x4);
      __m128 yCeil_x4 = _mm_min_ps(_mm_add_ps(yFloor_x4, one_x4), bitmapMaxY_x4);

      __m128 tX_x4 = _mm_sub_ps(bmPosX_x4, xFloor_x4);
      __m128 tY_x4 = _mm_sub_ps(bmPosY_x4, yFloor_x4);

      i32 orgIndex = (i32)(x + y * scrSize.x);
      u32* pixel = drawBuffer->pixel + orgIndex;

      __m128i destCol = _mm_loadu_si128((__m128i*)pixel);

      __m128 zBuffer_x4 = _mm_loadu_ps(cam->zBuffer + orgIndex);

      __m128 edgeMask = _mm_cmplt_ps(x_x4, cEndX_x4);
      __m128 depthMask = _mm_cmplt_ps(z_x4, zBuffer_x4);
      __m128 writeMaskPS = (_mm_and_ps(depthMask, edgeMask));
      __m128i writeMask = _mm_castps_si128(writeMaskPS);

      //NOTE: zBuffering
      __m128 minDepth = _mm_min_ps(zBuffer_x4, z_x4);
      __m128 depthOut = _mm_or_ps(_mm_and_ps(writeMaskPS, minDepth), _mm_andnot_ps(writeMaskPS, zBuffer_x4));
      _mm_storeu_ps(cam->zBuffer + orgIndex, depthOut);

      i32 xFloor = (i32)PS(xFloor_x4, 0);
      i32 yFloor = (i32)PS(yFloor_x4, 0);
      i32 xCeil = (i32)PS(xCeil_x4, 0);
      i32 yCeil = (i32)PS(yCeil_x4, 0);

      __m128i sampleA_x4 = _mm_loadu_si128((__m128i*) & bitmap->pixel[yFloor * bitmap->width + xFloor]);
      __m128i sampleB_x4 = _mm_loadu_si128((__m128i*) & bitmap->pixel[yFloor * bitmap->width + xCeil]);
      __m128i sampleC_x4 = _mm_loadu_si128((__m128i*) & bitmap->pixel[yCeil * bitmap->width + xFloor]);
      __m128i sampleD_x4 = _mm_loadu_si128((__m128i*) & bitmap->pixel[yCeil * bitmap->width + xCeil]);

      __m128i ffMask = _mm_set1_epi32(0xFF);

      __m128 sampleAA_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleA_x4, 24), ffMask));
      __m128 sampleAR_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleA_x4, 16), ffMask));
      __m128 sampleAG_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleA_x4, 8), ffMask));
      __m128 sampleAB_x4 = _mm_cvtepi32_ps(_mm_and_si128(sampleA_x4, ffMask));

      __m128 sampleBA_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleB_x4, 24), ffMask));
      __m128 sampleBR_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleB_x4, 16), ffMask));
      __m128 sampleBG_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleB_x4, 8), ffMask));
      __m128 sampleBB_x4 = _mm_cvtepi32_ps(_mm_and_si128(sampleB_x4, ffMask));

      __m128 sampleCA_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleC_x4, 24), ffMask));
      __m128 sampleCR_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleC_x4, 16), ffMask));
      __m128 sampleCG_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleC_x4, 8), ffMask));
      __m128 sampleCB_x4 = _mm_cvtepi32_ps(_mm_and_si128(sampleC_x4, ffMask));

      __m128 sampleDA_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleD_x4, 24), ffMask));
      __m128 sampleDR_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleD_x4, 16), ffMask));
      __m128 sampleDG_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleD_x4, 8), ffMask));
      __m128 sampleDB_x4 = _mm_cvtepi32_ps(_mm_and_si128(sampleD_x4, ffMask));

      __m128 destA_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(destCol, 24), ffMask));
      __m128 destR_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(destCol, 16), ffMask));
      __m128 destG_x4 = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(destCol, 8), ffMask));
      __m128 destB_x4 = _mm_cvtepi32_ps(_mm_and_si128(destCol, ffMask));

#define SQUARE_PS(M) _mm_mul_ps(M, M)
      sampleAR_x4 = SQUARE_PS(_mm_mul_ps(sampleAR_x4, inv255_x4));
      sampleBR_x4 = SQUARE_PS(_mm_mul_ps(sampleBR_x4, inv255_x4));
      sampleCR_x4 = SQUARE_PS(_mm_mul_ps(sampleCR_x4, inv255_x4));
      sampleDR_x4 = SQUARE_PS(_mm_mul_ps(sampleDR_x4, inv255_x4));

      sampleAG_x4 = SQUARE_PS(_mm_mul_ps(sampleAG_x4, inv255_x4));
      sampleBG_x4 = SQUARE_PS(_mm_mul_ps(sampleBG_x4, inv255_x4));
      sampleCG_x4 = SQUARE_PS(_mm_mul_ps(sampleCG_x4, inv255_x4));
      sampleDG_x4 = SQUARE_PS(_mm_mul_ps(sampleDG_x4, inv255_x4));

      sampleAB_x4 = SQUARE_PS(_mm_mul_ps(sampleAB_x4, inv255_x4));
      sampleBB_x4 = SQUARE_PS(_mm_mul_ps(sampleBB_x4, inv255_x4));
      sampleCB_x4 = SQUARE_PS(_mm_mul_ps(sampleCB_x4, inv255_x4));
      sampleDB_x4 = SQUARE_PS(_mm_mul_ps(sampleDB_x4, inv255_x4));

      sampleAA_x4 = _mm_mul_ps(sampleAA_x4, inv255_x4);
      sampleBA_x4 = _mm_mul_ps(sampleBA_x4, inv255_x4);
      sampleCA_x4 = _mm_mul_ps(sampleCA_x4, inv255_x4);
      sampleDA_x4 = _mm_mul_ps(sampleDA_x4, inv255_x4);

      __m128 sampleX1R = _mm_add_ps(sampleAR_x4, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleBR_x4, sampleAR_x4)));
      __m128 sampleX1G = _mm_add_ps(sampleAG_x4, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleBG_x4, sampleAG_x4)));
      __m128 sampleX1B = _mm_add_ps(sampleAB_x4, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleBB_x4, sampleAB_x4)));
      __m128 sampleX1A = _mm_add_ps(sampleAA_x4, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleBA_x4, sampleAA_x4)));

      __m128 sampleX2R = _mm_add_ps(sampleCR_x4, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleDR_x4, sampleCR_x4)));
      __m128 sampleX2G = _mm_add_ps(sampleCG_x4, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleDG_x4, sampleCG_x4)));
      __m128 sampleX2B = _mm_add_ps(sampleCB_x4, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleDB_x4, sampleCB_x4)));
      __m128 sampleX2A = _mm_add_ps(sampleCA_x4, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleDA_x4, sampleCA_x4)));

      __m128 texelR_x4 = _mm_add_ps(sampleX1R, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleX2R, sampleX1R)));
      __m128 texelG_x4 = _mm_add_ps(sampleX1G, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleX2G, sampleX1G)));
      __m128 texelB_x4 = _mm_add_ps(sampleX1B, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleX2B, sampleX1B)));
      __m128 texelA_x4 = _mm_add_ps(sampleX1A, _mm_mul_ps(tX_x4, _mm_sub_ps(sampleX2A, sampleX1A)));

      texelR_x4 = _mm_mul_ps(texelR_x4, lightMulR_x4);
      texelG_x4 = _mm_mul_ps(texelG_x4, lightMulG_x4);
      texelB_x4 = _mm_mul_ps(texelB_x4, lightMulB_x4);

      texelR_x4 = _mm_max_ps(texelR_x4, zero_x4);
      texelG_x4 = _mm_max_ps(texelG_x4, zero_x4);
      texelB_x4 = _mm_max_ps(texelB_x4, zero_x4);
      texelA_x4 = _mm_max_ps(texelA_x4, zero_x4);

      texelR_x4 = _mm_min_ps(texelR_x4, one_x4);
      texelG_x4 = _mm_min_ps(texelG_x4, one_x4);
      texelB_x4 = _mm_min_ps(texelB_x4, one_x4);
      texelA_x4 = _mm_min_ps(texelA_x4, one_x4);

      destR_x4 = SQUARE_PS(_mm_mul_ps(destR_x4, inv255_x4));
      destG_x4 = SQUARE_PS(_mm_mul_ps(destG_x4, inv255_x4));
      destB_x4 = SQUARE_PS(_mm_mul_ps(destB_x4, inv255_x4));
      destA_x4 = _mm_mul_ps(destA_x4, inv255_x4);

      __m128 invSA = _mm_sub_ps(one_x4, texelA_x4);

      __m128 blendedR = _mm_add_ps(texelR_x4, _mm_mul_ps(invSA, destR_x4));
      __m128 blendedG = _mm_add_ps(texelG_x4, _mm_mul_ps(invSA, destG_x4));
      __m128 blendedB = _mm_add_ps(texelB_x4, _mm_mul_ps(invSA, destB_x4));
      __m128 blendedA = _mm_add_ps(texelA_x4, _mm_mul_ps(invSA, destA_x4));

      blendedR = _mm_sqrt_ps(blendedR);
      blendedG = _mm_sqrt_ps(blendedG);
      blendedB = _mm_sqrt_ps(blendedB);

      blendedR = _mm_mul_ps(blendedR, one255_x4);
      blendedG = _mm_mul_ps(blendedG, one255_x4);
      blendedB = _mm_mul_ps(blendedB, one255_x4);
      blendedA = _mm_mul_ps(blendedA, one255_x4);

      __m128i intR = _mm_cvtps_epi32(blendedR);
      __m128i intG = _mm_cvtps_epi32(blendedG);
      __m128i intB = _mm_cvtps_epi32(blendedB);
      __m128i intA = _mm_cvtps_epi32(blendedA);

      __m128i sA = _mm_slli_epi32(intA, 24);
      __m128i sR = _mm_slli_epi32(intR, 16);
      __m128i sG = _mm_slli_epi32(intG, 8);
      __m128i sB = intB;

      __m128i pixelOut = _mm_or_si128(_mm_or_si128(sR, sG), _mm_or_si128(sB, sA));

#if 0
      pixelOut = _mm_set1_epi32(color.ToU32());
      __m128i maskedOut = _mm_or_si128(_mm_and_si128(writeMask, pixelOut), _mm_andnot_si128(writeMask, destCol));
#else
      __m128i maskedOut = _mm_or_si128(_mm_and_si128(writeMask, pixelOut), _mm_andnot_si128(writeMask, destCol));
#endif


      _mm_storeu_si128((__m128i*)pixel, maskedOut);

      END_TIMER_BLOCK_COUNTED(PixelFill, 4);
    }
  }

}

struct fill_flat_quad_work
{
  loaded_bitmap* drawBuffer;
  loaded_bitmap* bitmap;
  camera* cam;
  directional_light light;
  v3 faceNormal;
  v4 color;
  vertex_line lineL;
  vertex_line lineR;
  float startY;
  float endY;
};

WORK_ENTRY_CALLBACK(FillFlatQuadTexWork)
{
  fill_flat_quad_work* work = (fill_flat_quad_work*)data;
  FillFlatQuadTex(work->drawBuffer, work->bitmap, work->cam, work->light, work->faceNormal, work->color, work->lineL, work->lineR, work->startY, work->endY);
}

void DrawFlatQuadTex(platform_work_queue* queue, loaded_bitmap* drawBuffer, loaded_bitmap* bitmap, camera* cam, directional_light light, flat_quad bound, v3 faceNormal, v4 color)
{
  v2 scrSize = V2(drawBuffer->width, drawBuffer->height);

  vertex_line lineL;
  lineL.origin = bound.p0;
  lineL.dir = bound.p1 - bound.p0;
  vertex_line lineR;
  lineR.origin = bound.p2;
  lineR.dir = bound.p3 - bound.p2;

  float minY = Max(bound.p0.pos.y, 0.0f);
  float maxY = Min(bound.p1.pos.y, scrSize.y);

  const i32 partCount = 8;
  float stepY = (maxY - minY) * (1.0f / (float)partCount);
  fill_flat_quad_work workArr[partCount];
  for (i32 i = 0; i < partCount; ++i)
  {
    fill_flat_quad_work* work = workArr + i;
    float startY = Ceil(minY + ((float)i * stepY));
    float endY = Ceil(minY + ((float)(i + 1) * stepY));
    *work = { drawBuffer, bitmap, cam, light, faceNormal, color, lineL, lineR , startY, endY };

    PlatformAddWorkEntry(queue, FillFlatQuadTexWork, work);
  }
  PlatformCompleteAllWork(queue);
}

void DrawTriangle(platform_work_queue* queue, loaded_bitmap* drawBuffer, camera* cam, directional_light light, triangle tri, v3 faceNormals, v4 color, loaded_bitmap* bitmap)
{
  BEGIN_TIMER_BLOCK(DrawTriangle);

  i32 count = ARRAY_COUNT(tri.p);
  for (i32 i = 0; i < count; ++i)
  {
    for (i32 j = i + 1; j < count; ++j)
    {
      if (tri.p[i].pos.y > tri.p[j].pos.y)
      {
        Swap(&tri.p[i], &tri.p[j]);
      }
    }
  }

  for (i32 i = 0; i < count; ++i)
  {
    //NOTE: perspective divide
    tri.p[i].pos.xy = NDCPointToScreen(cam, tri.p[i].pos);
    float zInv = 1.0f / tri.p[i].pos.z;
    tri.p[i].uv *= zInv;
    tri.p[i].pos.z = zInv;
  }

  float tScr = (tri.p[1].pos.y - tri.p[0].pos.y) / (tri.p[2].pos.y - tri.p[0].pos.y);
  if (isnan(tScr)) return;

  v3 scrMiddle = Lerp(tri.p[0].pos, tri.p[2].pos, tScr);
  v2 uvMiddle = Lerp(tri.p[0].uv, tri.p[2].uv, tScr);

  v3 scrMiddleL;
  v2 uvMiddleL;

  v3 scrMiddleR;
  v2 uvMiddleR;

  if (scrMiddle.x < tri.p[1].pos.x)
  {
    scrMiddleL = scrMiddle;
    scrMiddleR = tri.p[1].pos;

    uvMiddleL = uvMiddle;
    uvMiddleR = tri.p[1].uv;
  }
  else
  {
    scrMiddleL = tri.p[1].pos;
    scrMiddleR = scrMiddle;

    uvMiddleL = tri.p[1].uv;
    uvMiddleR = uvMiddle;
  }

  flat_quad bound;
  bound.p0 = tri.p0;
  bound.p1 = { scrMiddleL, uvMiddleL };
  bound.p2 = tri.p0;
  bound.p3 = { scrMiddleR, uvMiddleR };
  DrawFlatQuadTex(queue, drawBuffer, bitmap, cam, light, bound, faceNormals, color);

  bound.p0 = { scrMiddleL, uvMiddleL };
  bound.p1 = tri.p2;
  bound.p2 = { scrMiddleR, uvMiddleR };
  bound.p3 = tri.p2;
  DrawFlatQuadTex(queue, drawBuffer, bitmap, cam, light, bound, faceNormals, color);

  END_TIMER_BLOCK(DrawTriangle)
}

void ProcessTriangle(platform_work_queue* queue, loaded_bitmap* drawBuffer, camera* cam, directional_light light, triangle tri, v3 faceNormal, loaded_bitmap* texture, v4 color)
{
  for (int i = 0; i < ARRAY_COUNT(tri.p); ++i)
  {
    //TODO: zClipping
    if (tri.p[i].pos.z < 0.0f) return;

    tri.p[i].pos = CameraPointToNDC(cam, tri.p[i].pos);
  }
  DrawTriangle(queue, drawBuffer, cam, light, tri, faceNormal, color, texture);
  DrawTriangle(queue, drawBuffer, cam, light, tri, faceNormal, color, texture);
}

void Draw(platform_work_queue* queue, loaded_bitmap* drawBuffer, directional_light light, camera* cam, vertex* ver, i32 verCount, i32* index, i32 indexCount, loaded_bitmap* texture, v4 color)
{
  v3* faceNormals = (v3*)_alloca(sizeof(v3) * indexCount / 3);
  for (int i = 0; i < indexCount; i += 3)
  {
    triangle tri;
    tri.p0 = ver[index[i]];
    tri.p1 = ver[index[i + 1]];
    tri.p2 = ver[index[i + 2]];

    faceNormals[i / 3] = -Cross(tri.p1.pos - tri.p0.pos, tri.p2.pos - tri.p0.pos);
  }

  for (int i = 0; i < verCount; ++i)
  {
    //NOTE: vertex shader
    ver[i].pos = WorldPointToCamera(cam, ver[i].pos);

    // char buf[256];
    // _snprintf_s(buf, sizeof(buf), "vertex%i: pos=%f, %f, %f", i, ver[i].pos.x, ver[i].pos.y, ver[i].pos.z);
    // OutputDebugStringA(buf);
  }

  for (int i = 0; i < indexCount; i += 3)
  {
    triangle tri;
    tri.p0 = ver[index[i]];
    tri.p1 = ver[index[i + 1]];
    tri.p2 = ver[index[i + 2]];

    v3 normal = Cross(tri.p1.pos - tri.p0.pos, tri.p2.pos - tri.p0.pos);

    //NOTE: backface culling
    if (Dot(tri.p0.pos, normal) > 0.0f)
    {
      ProcessTriangle(queue, drawBuffer, cam, light, tri, faceNormals[i / 3], texture, color);
    }
  }
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

      float edge1 = Dot(-Perp(xAxis), d);
      float edge2 = Dot(Perp(yAxis), d - yAxis);
      float edge3 = Dot(-Perp(yAxis), d - xAxis);
      float edge4 = Dot(Perp(xAxis), d - xAxis - yAxis);

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

static void RenderGroupOutput(platform_work_queue* queue, render_group* renderGroup, loaded_bitmap* drawBuffer)
{
  BEGIN_TIMER_BLOCK(RenderGroupOutput);

  for (int index = 0; index < renderGroup->pushBuffer.used;)
  {
    render_entry_header* header = (render_entry_header*)((u8*)renderGroup->pushBuffer.base + index);
    index += sizeof(*header);

    switch (header->type)
    {
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

      case RENDER_TYPE_render_entry_clear:
      {
        render_entry_clear* entry = (render_entry_clear*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        ClearBuffer(drawBuffer, entry->color.ToU32());
      } break;

      case RENDER_TYPE_render_entry_mesh:
      {
        render_entry_mesh* entry = (render_entry_mesh*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        Draw(queue, drawBuffer, entry->light, renderGroup->cam, entry->ver, entry->verCount, entry->index, entry->indexCount, entry->bitmap, entry->col);
      } break;

      // INVALID_DEFAULT_CASE;
    }
  }

  END_TIMER_BLOCK(RenderGroupOutput);
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

    v2 minP = WorldPointToScreen(renderGroup->cam, pos - halfSize) - V2(bitmap->width, bitmap->height) * bitmap->align;
    v2 maxP = WorldPointToScreen(renderGroup->cam, pos + halfSize) - V2(bitmap->width, bitmap->height) * bitmap->align;

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
    entry->minP = WorldPointToScreen(renderGroup->cam, pos - halfSize);
    entry->maxP = WorldPointToScreen(renderGroup->cam, pos + halfSize);
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
    entry->minP = WorldPointToScreen(renderGroup->cam, pos - halfSize);
    entry->maxP = WorldPointToScreen(renderGroup->cam, pos + halfSize);
    entry->color = col;
    entry->thickness = thickness;
  }

  return entry;
}

inline render_entry_mesh* RenderMesh(render_group* renderGroup, directional_light light, vertex* ver, i32 verCount, i32* index, i32 indexCount, v4 col, loaded_bitmap* bitmap)
{
  render_entry_mesh* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_mesh);
  if (entry)
  {
    entry->ver = ver;
    entry->verCount = verCount;
    entry->index = index;
    entry->indexCount = indexCount;
    entry->light = light;

    entry->col = col;
    entry->bitmap = bitmap;
  }

  return entry;
}

inline render_group* InitRenderGroup(memory_arena* arena, size_t maxPushBufferSize, camera* cam)
{
  render_group* result = PUSH_TYPE(arena, render_group);
  InitMemoryArena(&result->pushBuffer, maxPushBufferSize, PUSH_SIZE(arena, maxPushBufferSize));

  result->cam = cam;
  i32 pixelGuard = 4;
  i32 camSizeIndex = (i32)(cam->size.x * cam->size.y + pixelGuard);
  cam->zBuffer = PUSH_ARRAY(arena, float, camSizeIndex);
  for (int i = 0; i < cam->size.x * cam->size.y; ++i)
  {
    cam->zBuffer[i] = INFINITY;
  }

  return result;
}