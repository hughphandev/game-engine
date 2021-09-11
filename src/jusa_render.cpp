#include "jusa_render.h"
#include "jusa_math.cpp"

u32 color::ToU32()
{

  u32 red = this->r < 0.0f ? 0 : RoundToU32(this->r * 255.0f);
  u32 green = this->g < 0.0f ? 0 : RoundToU32(this->g * 255.0f);
  u32 blue = this->b < 0.0f ? 0 : RoundToU32(this->b * 255.0f);
  u32 alpha = this->a < 0.0f ? 0 : RoundToU32(this->a * 255.0f);

  return (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
}

inline u32 AlphaBlend(u32 source, u32 dest)
{
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

void DrawLine(game_offscreen_buffer* buffer, v2 from, v2 to, u32 lineColor)
{
  //NOTE: this will draw line in screen coord
  u32* pixel = (u32*)buffer->memory;
  if (from.x == to.x)
  {
    int startY = (int)Round(Min(from.y, to.y));
    int endY = (int)Round(Max(from.y, to.y));
    int x = (int)Round(from.x);
    int screenBound = buffer->width * buffer->height;
    for (int i = startY; i < endY; ++i)
    {
      int pixelIndex = i * buffer->width + x;
      if (pixelIndex >= 0 && pixelIndex < screenBound)
      {
        pixel[i * buffer->width + x] = lineColor;
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
      if (i >= 0 && i < buffer->width && y >= 0 && y < buffer->height)
      {
        pixel[y * buffer->width + i] = lineColor;
      }
    }
  }
}

static void DrawImage(game_offscreen_buffer* buffer, loaded_bitmap* image)
{
  u32* pixel = (u32*)buffer->memory;
  u32 width = (image->width < (u32)buffer->width) ? image->width : (u32)buffer->width;
  u32 height = (image->height < (u32)buffer->height) ? image->height : (u32)buffer->height;
  for (u32 y = 0; y < height; ++y)
  {
    for (u32 x = 0; x < width; ++x)
    {
      //NOTE: bitmap flipping;
      u32 sIndex = y * buffer->width + x;
      u32 dIndex = y * width + x;
      pixel[sIndex] = AlphaBlend(pixel[sIndex], image->pixel[dIndex]);
    }
  }
}

void ClearBuffer(game_offscreen_buffer* buffer, color c)
{
  u32* pixel = (u32*)buffer->memory;
  u32 col = c.ToU32();
  for (int y = 0; y < buffer->height; ++y)
  {
    for (int x = 0; x < buffer->width; ++x)
    {
      *pixel++ = col;
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

void DrawRectangle(game_offscreen_buffer* buffer, camera* cam, rec r, color c, brush_type brush = BRUSH_FILL)
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
    u32* mem = (u32*)buffer->memory;
    u32 col = c.ToU32();
    for (int x = xMin; x < xMax; x++)
    {
      for (int y = yMin; y < yMax; y++)
      {
        if (x >= 0 && x < buffer->width &&
            y >= 0 && y < buffer->height)
        {
          mem[y * buffer->width + x] = col;
        }
      }
    }
  }
  else if (brush == BRUSH_WIREFRAME)
  {
    // Draw tile bound
    DrawLine(buffer, { (float)xMin, (float)yMin }, { (float)xMax, (float)yMin }, c.ToU32());
    DrawLine(buffer, { (float)xMax, (float)yMin }, { (float)xMax, (float)yMax }, c.ToU32());
    DrawLine(buffer, { (float)xMax, (float)yMax }, { (float)xMin, (float)yMax }, c.ToU32());
    DrawLine(buffer, { (float)xMin, (float)yMax }, { (float)xMin, (float)yMin }, c.ToU32());
  }
}

void DrawRectSlowly(game_offscreen_buffer* buffer, camera* cam, v2 origin, v2 xAxis, v2 yAxis, color c, loaded_bitmap* texture, brush_type = BRUSH_FILL)
{
  v2 screenOrigin = WorldPointToScreen(cam, origin);
  v2 screenXAxis = V2(xAxis.x, -xAxis.y) * cam->pixelPerMeter;
  v2 screenYAxis = V2(yAxis.x, -yAxis.y) * cam->pixelPerMeter;

  float invSqrlengthX = 1.0f / (Sqr(screenXAxis.x) + Sqr(screenXAxis.y));
  float invSqrlengthy = 1.0f / (Sqr(screenYAxis.x) + Sqr(screenYAxis.y));

  u32* pixel = (u32*)buffer->memory;

  float xList[] = { screenOrigin.x, screenOrigin.x + screenXAxis.x, screenOrigin.x + screenYAxis.x, screenOrigin.x + screenXAxis.x + screenYAxis.x };
  int xCount = ARRAY_COUNT(xList);

  float yList[] = { screenOrigin.y, screenOrigin.y + screenXAxis.y, screenOrigin.y + screenYAxis.y, screenOrigin.y + screenXAxis.y + screenYAxis.y };
  int yCount = ARRAY_COUNT(yList);

  int xMin = Min(xList, xCount) >= 0 ? (int)Floor(Min(xList, xCount)) : 0;
  int xMax = Max(xList, xCount) < buffer->width ? (int)Ceil(Max(xList, xCount)) : buffer->width - 1;
  int yMin = Min(yList, yCount) >= 0 ? (int)Floor(Min(yList, yCount)) : 0;
  int yMax = Max(xList, yCount) < buffer->height ? (int)Ceil(Max(yList, yCount)) : buffer->height - 1;

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
        float u = Inner(d, screenXAxis) * invSqrlengthX;
        float v = Inner(d, screenYAxis) * invSqrlengthy;

        ASSERT(u >= 0 && u < 1);
        ASSERT(v >= 0 && v < 1);

        i32 xPixel = (i32)Round(u * (float)(texture->width - 1));
        i32 yPixel = (i32)Round(v * (float)(texture->height - 1));

        u32 col = (u32)texture->pixel[yPixel * texture->width + xPixel];

        pixel[y * buffer->width + x] = col;
      }
    }
  }
}

inline void DrawTexture(game_offscreen_buffer* buffer, camera* cam, rec bound, loaded_bitmap* texture)
{
  if (texture == 0)
  {
    DrawRectangle(buffer, cam, bound, { 1.0f, 1.0f, 1.0f, 1.0f });
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

  u32* scrPixel = (u32*)buffer->memory;
  for (int y = yMin; y < yMax; y++)
  {
    for (int x = xMin; x < xMax; x++)
    {
      if (x >= 0 && x < buffer->width &&
          y >= 0 && y < buffer->height)
      {
        //TODO: fix pixel scaling
        u32 pX = (u32)(((float)(x - xMin) / (float)(xMax - xMin)) * texture->width);
        u32 pY = (u32)(((float)(y - yMin) / (float)(yMax - yMin)) * texture->height);

        u32 scrIndex = y * buffer->width + x;
        scrPixel[scrIndex] = AlphaBlend(scrPixel[scrIndex], pixel[pY * texture->width + pX]);
      }
    }
  }
}

static void RenderGroupOutput(render_group* renderGroup, game_offscreen_buffer* drawBuffer)
{

  for (int index = 0; index < renderGroup->pushBuffer.used;)
  {
    render_entry_header* header = (render_entry_header*)((u8*)renderGroup->pushBuffer.base + index);

    switch (header->type)
    {
      case RENDER_TYPE_render_entry_clear:
      {
        render_entry_clear* entry = (render_entry_clear*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        ClearBuffer(drawBuffer, entry->col);
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

        DrawTexture(drawBuffer, renderGroup->cam, { entry->pos, entry->size }, entry->texture);

      } break;

      case RENDER_TYPE_render_entry_coordinate_system:
      {
        render_entry_coordinate_system* entry = (render_entry_coordinate_system*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        v2 size = { 0.1f, 0.1f };
        color yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
        DrawRectangle(drawBuffer, renderGroup->cam, { entry->origin.xy, size }, yellow, BRUSH_FILL);
        DrawRectangle(drawBuffer, renderGroup->cam, { entry->origin.xy + entry->xAxis.xy, size }, yellow, BRUSH_FILL);
        DrawRectangle(drawBuffer, renderGroup->cam, { entry->origin.xy + entry->yAxis.xy, size }, yellow, BRUSH_FILL);

        rec r = { entry->origin.xy + (entry->xAxis.xy / 2) + (entry->yAxis.xy / 2), {1, 1} };
        DrawRectSlowly(drawBuffer, renderGroup->cam, entry->origin.xy, entry->xAxis.xy, entry->yAxis.xy, entry->col, entry->texture);

      } break;

      INVALID_DEFAULT_CASE;
    }
  }
}

void* PushRenderElement_(render_group* renderGroup, size_t size, render_entry_type type)
{
  render_entry_header* result = (render_entry_header*)PushSize_(&renderGroup->pushBuffer, size);

  if (result)
  {
    result->type = type;
  }
  else
  {
    INVALID_CODE_PATH;
  }

  return result;
}
#define PUSH_RENDER_ELEMENT(renderGroup, type) (type*)PushRenderElement_(renderGroup, sizeof(type), RENDER_TYPE_##type) 

inline render_entry_bitmap* PushBitmap(render_group* renderGroup, loaded_bitmap* texture, v2 pos, v2 size, color col = { 1.0f, 1.0f, 1.0f, 1.0f })
{
  render_entry_bitmap* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_bitmap);
  entry->texture = texture;
  entry->pos = pos;
  entry->size = size;
  entry->col = col;

  return entry;
}

inline render_entry_rectangle* PushRect(render_group* renderGroup, v2 pos, v2 size, color col, brush_type brush = BRUSH_FILL)
{
  render_entry_rectangle* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_rectangle);
  entry->pos = pos;
  entry->size = size;
  entry->col = col;
  entry->brush = brush;

  return entry;
}

inline render_entry_coordinate_system* CoordinateSystem(render_group* RenderGroup, v3 origin, v3 xAxis, v3 yAxis, v3 zAxis, color col, loaded_bitmap* texture)
{
  render_entry_coordinate_system* entry = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_coordinate_system);
  entry->origin = origin;
  entry->xAxis = xAxis;
  entry->yAxis = yAxis;
  entry->zAxis = zAxis;
  entry->col = col;
  entry->texture = texture;

  return entry;
}

inline render_group* InitRenderGroup(memory_arena* arena, size_t maxPushBufferSize, camera* cam)
{
  render_group* result = PUSH_TYPE(arena, render_group);
  InitMemoryArena(&result->pushBuffer, maxPushBufferSize, PUSH_SIZE(arena, maxPushBufferSize));
  result->cam = cam;

  return result;
}