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

bilinear_sample BilinearSample(loaded_bitmap* bitmap, u32 x, u32 y)
{
  bilinear_sample result;
  result.a = bitmap->pixel[y * bitmap->width + x];
  result.b = bitmap->pixel[y * bitmap->width + (x + 1)];
  result.c = bitmap->pixel[(y + 1) * bitmap->width + x];
  result.d = bitmap->pixel[(y + 1) * bitmap->width + (x + 1)];
  return result;
}

v4 SRGBBilinearBlend(bilinear_sample sample, float tX, float tY)
{
  //NOTE: input in sRGB, output in linear
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

inline v4 AlphaBlend(v4 col, v4 baseCol)
{
  float invSA = (1 - col.a);
  v4 result = invSA * baseCol + col;
  return result;
}

inline u32 AlphaBlend(u32 source, u32 dest)
{
  v4 d = UnpackToRGBA255(dest) / 255.0f;
  v4 s = UnpackToRGBA255(source) / 255.0f;

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

static void DrawImage(loaded_bitmap* buffer, loaded_bitmap* image, v4 col = { 1.0f, 1.0f, 1.0f })
{
  u32* pixel = buffer->pixel;
  if (image)
  {
    int width = (image->width < buffer->width) ? image->width : buffer->width;
    int height = (image->height < buffer->height) ? image->height : buffer->height;
    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        //NOTE: bitmap flipping;
        int dIndex = y * buffer->width + x;
        int sIndex = y * width + x;
        pixel[dIndex] = AlphaBlend(image->pixel[sIndex], pixel[dIndex]);
      }
    }
  }
}

static void DrawScreenRect(loaded_bitmap* buffer, v2 min, v2 max, v4 color)
{
  u32* pixel = buffer->pixel;

  min = Clamp(min, V2(0, 0), V2((float)buffer->width, (float)buffer->height));
  max = Clamp(max, V2(0, 0), V2((float)buffer->width, (float)buffer->height));

  for (int y = (int)min.y; y < (int)max.y; ++y)
  {
    for (int x = (int)min.x; x < (int)max.x; ++x)
    {
      //NOTE: bitmap flipping;
      int index = y * buffer->width + x;
      pixel[index] = color.ToU32();
    }
  }
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

      //TODO: actually generate sphere!!
      v3 normal = V3(0.0f, 0.707106781187f, 0.707106781187f);
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

inline v2 WorldPointToScreen(camera* cam, v2 point)
{
  //TODO: Test
  v2 relPos = point - cam->pos.xy;
  v2 screenCoord = { relPos.x * (float)cam->pixelPerMeter, -relPos.y * (float)cam->pixelPerMeter };
  return screenCoord + cam->offSet.xy;
}

inline v2 ScreenPointToWorld(camera* cam, v2 point)
{
  //TODO: Test
  //TODO: fix offset of the pointer to the top-left of the window or change mouseX, mouseY relative to the top-left of the window
  v2 relPos = point - cam->offSet.xy;
  v2 worldCoord = { relPos.x / (float)cam->pixelPerMeter, -relPos.y / (float)cam->pixelPerMeter };
  return worldCoord + cam->pos.xy;
}

void DrawRectangle(loaded_bitmap* drawBuffer, camera* cam, rec r, v4 c, brush_type brush = BRUSH_FILL)
{
  v2 minBound = WorldPointToScreen(cam, r.GetMinBound());
  v2 maxBound = WorldPointToScreen(cam, r.GetMaxBound());

  if (minBound.x > maxBound.x) Swap(&minBound.x, &maxBound.x);
  if (minBound.y > maxBound.y) Swap(&minBound.y, &maxBound.y);

  int xMin = (int)Round(minBound.x);
  int yMin = (int)Round(minBound.y);
  int xMax = (int)Round(maxBound.x);
  int yMax = (int)Round(maxBound.y);
  if (brush == BRUSH_FILL)
  {
    u32* mem = (u32*)drawBuffer->pixel;
    u32 col = c.ToU32();
    for (int x = xMin; x < xMax; x++)
    {
      for (int y = yMin; y < yMax; y++)
      {
        if (x >= 0 && x < drawBuffer->width &&
            y >= 0 && y < drawBuffer->height)
        {
          mem[y * drawBuffer->width + x] = col;
        }
      }
    }
  }
  else if (brush == BRUSH_WIREFRAME)
  {
    // Draw tile bound
    DrawLine(drawBuffer, { (float)xMin, (float)yMin }, { (float)xMax, (float)yMin }, c.ToU32());
    DrawLine(drawBuffer, { (float)xMax, (float)yMin }, { (float)xMax, (float)yMax }, c.ToU32());
    DrawLine(drawBuffer, { (float)xMax, (float)yMax }, { (float)xMin, (float)yMax }, c.ToU32());
    DrawLine(drawBuffer, { (float)xMin, (float)yMax }, { (float)xMin, (float)yMin }, c.ToU32());
  }
}

v3 SampleEnvMap(v2 screenSpaceUV, v3 sampleDirection, float roughness, environment_map* map)
{
  u32 LODIndex = (u32)(roughness * (float)(ARRAY_COUNT(map->LOD) - 1) + 0.5f);
  ASSERT(LODIndex < ARRAY_COUNT(map->LOD));

  loaded_bitmap* LOD = &map->LOD[LODIndex];

  ASSERT(sampleDirection.y > 0.0f);
  float distanceFromMapInz = 1.0f;
  float UVsPerMeter = 0.01f;
  float c = (UVsPerMeter * distanceFromMapInz) / sampleDirection.y;
  //TODO: make sure we know what direction z should go in y
  v2 offset = c * V2(sampleDirection.x, sampleDirection.z);
  v2 UV = offset + screenSpaceUV;

  UV.x = Clamp01(UV.x);
  UV.y = Clamp01(UV.y);

  //TODO: actually calculate x and y
  float xPixel = (UV.x * (float)(LOD->width - 2));
  float yPixel = (UV.y * (float)(LOD->height - 2));


  i32 x = (i32)(xPixel);
  i32 y = (i32)(yPixel);

  float tX = xPixel - x;
  float tY = yPixel - y;

  bilinear_sample sample = BilinearSample(LOD, x, y);
  v3 result = SRGBBilinearBlend(sample, tX, tY).xyz;

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


void DrawRectSlowly(loaded_bitmap* drawBuffer, camera* cam, v2 origin, v2 xAxis, v2 yAxis, v4 c, loaded_bitmap* texture, loaded_bitmap* normalMap, environment_map* top, environment_map* middle, environment_map* bottom)
{
  v2 screenOrigin = WorldPointToScreen(cam, origin);
  v2 screenXAxis = V2(xAxis.x, -xAxis.y) * cam->pixelPerMeter;
  v2 screenYAxis = V2(yAxis.x, -yAxis.y) * cam->pixelPerMeter;

  float invSqrlengthX = 1.0f / (Sqr(screenXAxis.x) + Sqr(screenXAxis.y));
  float invSqrlengthy = 1.0f / (Sqr(screenYAxis.x) + Sqr(screenYAxis.y));

  u32* pixel = (u32*)drawBuffer->pixel;

  float xList[] = { screenOrigin.x, screenOrigin.x + screenXAxis.x, screenOrigin.x + screenYAxis.x, screenOrigin.x + screenXAxis.x + screenYAxis.x };
  int xCount = ARRAY_COUNT(xList);

  float yList[] = { screenOrigin.y, screenOrigin.y + screenXAxis.y, screenOrigin.y + screenYAxis.y, screenOrigin.y + screenXAxis.y + screenYAxis.y };
  int yCount = ARRAY_COUNT(yList);

  int xMin = Min(xList, xCount) >= 0 ? (int)Floor(Min(xList, xCount)) : 0;
  int xMax = Max(xList, xCount) < drawBuffer->width ? (int)Ceil(Max(xList, xCount)) : drawBuffer->width - 1;
  int yMin = Min(yList, yCount) >= 0 ? (int)Floor(Min(yList, yCount)) : 0;
  int yMax = Max(xList, yCount) < drawBuffer->height ? (int)Ceil(Max(yList, yCount)) : drawBuffer->height - 1;

  for (int x = xMin; x < xMax; x++)
  {
    for (int y = yMin; y < yMax; y++)
    {
      v2 p = V2((float)x, (float)y);
      v2 d = p - screenOrigin;

      float edge1 = Inner(Perp(screenXAxis), d);
      float edge2 = Inner(-Perp(screenYAxis), d);
      float edge3 = Inner(Perp(screenYAxis), d - screenXAxis);
      float edge4 = Inner(-Perp(screenXAxis), d - screenYAxis);

      if (edge1 < 0 && edge2 < 0 && edge3 < 0 && edge4 < 0)
      {
        v2 screenSpaceUV = p / V2((float)drawBuffer->width - 1, (float)drawBuffer->height - 1);

        float u = Inner(d, screenXAxis) * invSqrlengthX;
        float v = Inner(d, screenYAxis) * invSqrlengthy;

        ASSERT(u >= 0 && u <= 1);
        ASSERT(v >= 0 && v <= 1);

        ASSERT(texture->width > 2);
        ASSERT(texture->height > 2);

        float xPixel = (u * (float)(texture->width - 2));
        float yPixel = (v * (float)(texture->height - 2));

        i32 xFloor = (i32)(xPixel);
        i32 yFloor = (i32)(yPixel);

        float tX = xPixel - xFloor;
        float tY = yPixel - yFloor;

        bilinear_sample texelSample = BilinearSample(texture, xFloor, yFloor);
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
          //TODO: do we really need this?
          normal = Normalize(normal);

          //NOTE: assume eye vector point in (0, 0, 1) screen space direction
          v3 bounceDirection = 2.0f * normal.z * normal.xyz;
          bounceDirection.z -= 1.0f;

          //TODO: rotate normal base on x,y axis

          float tEnvMap = bounceDirection.y;
          float tFarMap = 0.0f;
          environment_map* farMap = middle;
          if (tEnvMap < -0.5f)
          {
            farMap = bottom;
            tFarMap = -1.0f - (2.0f * tEnvMap);
            bounceDirection.y = -bounceDirection.y;
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
            v3 farMapCol = SampleEnvMap(screenSpaceUV, bounceDirection, normal.w, farMap);
            lightCol = Lerp(lightCol, farMapCol, tFarMap);
          }

          texel.rgb = texel.rgb + texel.a * lightCol;
        }
        texel = Hadamard(texel, c);
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

inline void DrawBitmap(loaded_bitmap* drawBuffer, camera* cam, rec bound, loaded_bitmap* texture)
{
  if (texture == 0)
  {
    DrawRectangle(drawBuffer, cam, bound, { 1.0f, 1.0f, 1.0f, 1.0f });
    return;
  }

  v2 minBound = WorldPointToScreen(cam, bound.GetMinBound());
  v2 maxBound = WorldPointToScreen(cam, bound.GetMaxBound());

  int xMin = (int)Round(minBound.x);
  int yMin = (int)Round(maxBound.y);
  int xMax = (int)Round(maxBound.x);
  int yMax = (int)Round(minBound.y);

  ASSERT(xMax >= xMin);
  ASSERT(yMax >= yMin);

  u32* pixel = texture->pixel;

  u32* destPixel = drawBuffer->pixel;
  for (int y = yMin; y < yMax; y++)
  {
    for (int x = xMin; x < xMax; x++)
    {
      if (x >= 0 && x < drawBuffer->width &&
          y >= 0 && y < drawBuffer->height)
      {
        //TODO: fix pixel scaling
        u32 pX = (u32)(((float)(x - xMin) / (float)(xMax - xMin)) * texture->width);
        u32 pY = (u32)(((float)(y - yMin) / (float)(yMax - yMin)) * texture->height);

        u32 scrIndex = y * drawBuffer->width + x;
        destPixel[scrIndex] = AlphaBlend(pixel[pY * texture->width + pX], destPixel[scrIndex]);
      }
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

        DrawRectangle(drawBuffer, renderGroup->cam, { entry->pos, entry->size }, entry->col, entry->brush);
      } break;

      case RENDER_TYPE_render_entry_bitmap:
      {
        render_entry_bitmap* entry = (render_entry_bitmap*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        DrawBitmap(drawBuffer, renderGroup->cam, { entry->pos, entry->size }, entry->texture);

      } break;

      case RENDER_TYPE_render_entry_coordinate_system:
      {
        render_entry_coordinate_system* entry = (render_entry_coordinate_system*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        v2 size = { 0.1f, 0.1f };
        v4 yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
        DrawRectangle(drawBuffer, renderGroup->cam, { entry->origin.xy, size }, yellow, BRUSH_FILL);
        DrawRectangle(drawBuffer, renderGroup->cam, { entry->origin.xy + entry->xAxis.xy, size }, yellow, BRUSH_FILL);
        DrawRectangle(drawBuffer, renderGroup->cam, { entry->origin.xy + entry->yAxis.xy, size }, yellow, BRUSH_FILL);
        DrawRectangle(drawBuffer, renderGroup->cam, { entry->origin.xy + entry->xAxis.xy + entry->yAxis.xy, size }, yellow, BRUSH_FILL);

        rec r = { entry->origin.xy + (entry->xAxis.xy / 2) + (entry->yAxis.xy / 2), {1, 1} };
        DrawRectSlowly(drawBuffer, renderGroup->cam, entry->origin.xy, entry->xAxis.xy, entry->yAxis.xy, entry->col, entry->texture, entry->normalMap, entry->top, entry->middle, entry->bottom);

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

inline render_entry_bitmap* PushBitmap(render_group* renderGroup, loaded_bitmap* texture, v2 pos, v2 size, v4 col = { 1.0f, 1.0f, 1.0f, 1.0f })
{
  render_entry_bitmap* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_bitmap);
  if (entry)
  {
    entry->texture = texture;
    entry->pos = pos;
    entry->size = size;
    entry->color = col;
  }

  return entry;
}

inline render_entry_rectangle* PushRect(render_group* renderGroup, v2 pos, v2 size, v4 col, brush_type brush = BRUSH_FILL)
{
  render_entry_rectangle* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_rectangle);
  if (entry)
  {
    entry->pos = pos;
    entry->size = size;
    entry->col = col;
    entry->brush = brush;
  }

  return entry;
}

inline render_entry_coordinate_system* CoordinateSystem(render_group* RenderGroup, v3 origin, v3 xAxis, v3 yAxis, v3 zAxis, v4 col, loaded_bitmap* texture, loaded_bitmap* normalMap, environment_map* top, environment_map* middle, environment_map* bottom)
{
  render_entry_coordinate_system* entry = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_coordinate_system);
  if (entry)
  {
    entry->origin = origin;
    entry->xAxis = xAxis;
    entry->yAxis = yAxis;
    entry->zAxis = zAxis;
    entry->col = col;
    entry->texture = texture;
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