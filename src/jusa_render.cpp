#include "jusa_render.h"
#include "jusa_math.cpp"

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
      pixel[sIndex] = LinearBlend(pixel[sIndex], image->pixel[dIndex]);
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
        scrPixel[scrIndex] = LinearBlend(scrPixel[scrIndex], pixel[pY * texture->width + pX]);
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
      case RENDER_ENTRY_CLEAR:
      {
        render_entry_clear* entry = (render_entry_clear*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        ClearBuffer(drawBuffer, entry->col);
      } break;

      case RENDER_ENTRY_RECTANGLE:
      {
        render_entry_rectangle* entry = (render_entry_rectangle*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        DrawRectangle(drawBuffer, renderGroup->cam, { entry->pos, entry->size }, entry->col, entry->brush);
      } break;

      case RENDER_ENTRY_BITMAP:
      {
        render_entry_bitmap* entry = (render_entry_bitmap*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*entry);

        DrawTexture(drawBuffer, renderGroup->cam, { entry->pos, entry->size }, entry->texture);

      } break;

      INVALID_DEFAULT_CASE;
    }
  }
}

void* PushRenderElement_(render_group* renderGroup, size_t size)
{
  void* result = PushSize_(&renderGroup->pushBuffer, size);

  return result;
}
#define PUSH_RENDER_ELEMENT(renderGroup, type) (type*)PushRenderElement_(renderGroup, sizeof(type))

inline render_entry_bitmap* PushBitmap(render_group* renderGroup, loaded_bitmap* texture, v2 pos, v2 size, color col)
{
  render_entry_bitmap* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_bitmap);
  entry->header.type = RENDER_ENTRY_BITMAP;
  entry->texture = texture;
  entry->pos = pos;
  entry->size = size;
  entry->col = col;

  return entry;
}

inline render_entry_rectangle* PushRect(render_group* renderGroup, v2 pos, v2 size, color col, brush_type brush = BRUSH_FILL)
{
  render_entry_rectangle* entry = PUSH_RENDER_ELEMENT(renderGroup, render_entry_rectangle);
  entry->header.type = RENDER_ENTRY_RECTANGLE;
  entry->pos = pos;
  entry->size = size;
  entry->col = col;
  entry->brush = brush;

  return entry;
}

inline render_group* InitRenderGroup(memory_arena* arena, size_t maxPushBufferSize, camera* cam)
{
  render_group* result = PUSH_TYPE(arena, render_group);
  InitMemoryArena(&result->pushBuffer, maxPushBufferSize, PUSH_SIZE(arena, maxPushBufferSize));
  result->cam = cam;

  return result;
}