#include "game.h"
#include "jusa_math.cpp"

void RenderScreen(game_offscreen_buffer* buffer, u32 color);
void DrawRectangle(game_offscreen_buffer* buffer, u32 color, rec r, game_camera cam);

#define PI 3.141592653589793f

v2 rec::GetMinBound()
{
  return { this->pos.x - (this->width / 2.0f), this->pos.y - (this->height / 2.0f) };
}

v2 rec::GetMaxBound()
{
  return { this->pos.x + (this->width / 2.0f), this->pos.y + (this->height / 2.0f) };
}

bool IsBoxOverlapping(rec a, rec b)
{
  bool result = false;
  v2 aMin = a.GetMinBound();
  v2 aMax = a.GetMaxBound();
  v2 bMin = b.GetMinBound();
  v2 bMax = b.GetMaxBound();

  result = (aMin.x < bMax.x) && (aMax.x > bMin.x) && (aMax.y > bMin.y) && (aMin.y < bMax.y);

  return result;
}


void Swap(float* l, float* r)
{
  float temp = *l;
  *l = *r;
  *r = temp;
}

bool RayToRec(v2 rayOrigin, v2 rayVec, rec r, v2* contactPoint, v2* contactNormal)
{
  if (rayVec == 0.0f) return false;

  v2 minPos = r.GetMinBound();
  v2 maxPos = r.GetMaxBound();

  v2 tNear = (minPos - rayOrigin) / rayVec;
  v2 tFar = (maxPos - rayOrigin) / rayVec;

  if (tNear.x > tFar.x) Swap(&tNear.x, &tFar.x);
  if (tNear.y > tFar.y) Swap(&tNear.y, &tFar.y);

  if (tNear.x > tFar.y || tNear.y > tFar.x) return false;

  float tHitNear = Max(tNear.x, tNear.y);
  float tHitFar = Min(tFar.x, tFar.y);

  //Note: check if value is a floating point number!
  if (tHitNear != tHitNear) return false;

  if (tHitFar < 0.0f) return false;
  if (tHitNear > 1.0f || tHitNear < 0.0f) return false;

  contactPoint->x = rayOrigin.x + (tHitNear * rayVec.x);
  contactPoint->y = rayOrigin.y + (tHitNear * rayVec.y);

  if (tNear.x > tNear.y)
  {
    if (rayVec.x > 0.0)
    {
      *contactNormal = { -1.0f, 0.0f };
    }
    else
    {
      *contactNormal = { 1.0f, 0.0f };
    }
  }
  else
  {
    if (rayVec.y < 0.0f)
    {
      *contactNormal = { 0.0f, 1.0f };
    }
    else
    {
      *contactNormal = { 0.0f, -1.0f };
    }
  }

  return true;
}

void DrawLine(game_offscreen_buffer* buffer, v2 from, v2 to, u32 lineColor)
{
  u32* pixel = (u32*)buffer->memory;
  if (from.x == to.x)
  {
    int startY = RoundToInt(Min(from.y, to.y));
    int endY = RoundToInt(Max(from.y, to.y));
    int x = RoundToInt(from.x);
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
    int startX = RoundToInt(Min(from.x, to.x));
    int endX = RoundToInt(Max(from.x, to.x));
    for (int i = startX; i < endX; ++i)
    {
      int y = RoundToInt(a * (float)i + b);
      if (i >= 0 && i < buffer->width && y >= 0 && y < buffer->height)
      {
        pixel[y * buffer->width + i] = lineColor;
      }
    }
  }
}


void MoveAndSlide(rec* target, v2 moveVec, game_world* world)
{
  for (u32 i = 0; i < world->objCount; ++i)
  {
    rec imRec = { world->obj[i].pos, world->obj[i].size + target->size };

    v2 contactNormal = {};
    v2 contactPoint = {};

    if (RayToRec(target->pos, moveVec, imRec, &contactPoint, &contactNormal))
    {
      //Note: only true with aabb
      //TODO: solution for generic case if needed
      v2 newMoveVec = {};
      newMoveVec.x = (contactNormal.x != 0.0f) ? 0 : moveVec.x;
      newMoveVec.y = (contactNormal.y != 0.0f) ? 0 : moveVec.y;
      target->pos = contactPoint + newMoveVec;
      return;
    }
  }
  target->pos = target->pos + moveVec;
}

#define PUSH_ARRAY(pool, type, count) (type*)PushSize_(pool, (sizeof(type) * count))
#define PUSH_TYPE(pool, type) (type*)PushSize_(pool, sizeof(type))
static void* PushSize_(memory_pool* pool, size_t bytes)
{
  ASSERT((pool->size - pool->used) >= bytes);

  void* result = (u8*)pool->base + pool->used;
  pool->used += bytes;
  return result;
}

static void InitializeMemoryPool(memory_pool* pool, size_t size, void* base)
{
  pool->base = (u8*)base;
  pool->size = size;
  pool->used = 0;
}

inline rec GetTileBound(game_world* world, u32 x, u32 y)
{
  rec result = {};

  result.size = world->tileSizeInMeter;


  v2 halfTileMapSizeInMeter = (world->tileSizeInMeter * V2((float)world->tileCountX, (float)world->tileCountY)) / 2.0f;

  result.pos = V2((result.height * (float)x), (result.width * (float)y)) - halfTileMapSizeInMeter;

  return result;
}

inline img DEBUGLoadBMP(game_memory* mem, game_state* gameState)
{
  img result;

  debug_read_file_result file = mem->DEBUGPlatformReadFile("build/test.bmp");
  if (file.contentSize > 0)
  {
    bmp_header* header = (bmp_header*)file.contents;
    u32* filePixel = (u32*)((u8*)file.contents + header->offSet);

    result.pixel = PUSH_ARRAY(&gameState->pool, u32, header->width * header->height);
    result.width = header->width;
    result.height = header->height;
    for (u32 y = 0; y < header->height; ++y)
    {
      for (u32 x = 0; x < header->width; ++x)
      {
        u32 indexMem = y * header->width + x;
        u32 index = (header->height - y - 1) * header->width + x;
        u32 red = filePixel[index] & header->redMask;
        u32 green = filePixel[index] & header->greenMask;
        u32 blue = filePixel[index] & header->blueMask;
        u32 alpha = filePixel[index] & header->alphaMask;

        while ((red >> 8) != 0)
        {
          red = (red >> 8);
        }
        while ((blue >> 8) != 0)
        {
          blue = (blue >> 8);
        }
        while ((green >> 8) != 0)
        {
          green = (green >> 8);
        }
        while ((alpha >> 8) != 0)
        {
          alpha = (alpha >> 8);
        }

        result.pixel[indexMem] = ((alpha << 24) | (red << 16) | (green << 8) | (blue));
      }
    }
  }
  return result;
}

static void DEBUGDrawBMP(game_offscreen_buffer* buffer, u32* bmpPixel, u32 width, u32 height)
{
  u32* pixel = (u32*)buffer->memory;
  width = (width < (u32)buffer->width) ? width : (u32)buffer->width;
  height = (height < (u32)buffer->height) ? height : (u32)buffer->height;
  for (u32 y = 0; y < height; ++y)
  {
    for (u32 x = 0; x < width; ++x)
    {
      pixel[y * buffer->width + x] = bmpPixel[y * width + x];
    }
  }
}

inline v2 WorldPointToScreen(game_camera cam, v2 point)
{
  v2 relPos = point - cam.pos;
  v2 screenCoord = { relPos.x * (float)cam.pixelPerMeter, -relPos.y * (float)cam.pixelPerMeter };
  return screenCoord + cam.offSet;
}

inline void DrawTile(game_offscreen_buffer* buffer, tile tile, game_camera cam)
{
  v2 minBound = WorldPointToScreen(cam, tile.bound.GetMinBound());
  v2 maxBound = WorldPointToScreen(cam, tile.bound.GetMaxBound());

  int xMin = RoundToInt(minBound.x);
  int yMin = RoundToInt(maxBound.y);
  int xMax = RoundToInt(maxBound.x);
  int yMax = RoundToInt(minBound.y);

  ASSERT(xMax >= xMin);
  ASSERT(yMax >= yMin);

  u32* pixel = tile.texture->pixel;

  u32* mem = (u32*)buffer->memory;
  for (int y = yMin; y < yMax; y++)
  {
    for (int x = xMin; x < xMax; x++)
    {
      if (x >= 0 && x < buffer->width &&
          y >= 0 && y < buffer->height)
      {
        //TODO: fix pixel scaling
        u32 pX = (u32)(((float)(x - xMin) / (float)(xMax - xMin)) * tile.texture->width);
        u32 pY = (u32)(((float)(y - yMin) / (float)(yMax - yMin)) * tile.texture->height);
        mem[y * buffer->width + x] = pixel[pY * tile.texture->width + pX];
      }
    }
  }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  game_state* gameState = (game_state*)gameMemory->permanentStorage;
  if (!gameMemory->initialized)
  {
    InitializeMemoryPool(&gameState->pool, gameMemory->permanentStorageSize - sizeof(game_state), (u8*)gameMemory->permanentStorage + sizeof(game_state));

    gameState->background = DEBUGLoadBMP(gameMemory, gameState);
    gameState->world = PUSH_TYPE(&gameState->pool, game_world);
    game_world* world = gameState->world;
    world->cam = PUSH_TYPE(&gameState->pool, game_camera);
    world->cam->pixelPerMeter = buffer->height / 9.0f;
    world->cam->offSet = { buffer->width / 2.0f, buffer->height / 2.0f };

    world->tileCountX = 10;
    world->tileCountY = 10;
    world->tileSizeInMeter = { 1.0f, 1.0f };
    world->tileMap = PUSH_ARRAY(&gameState->pool, tile, world->tileCountX * world->tileCountY);

    for (u32 i = 0; i < world->tileCountX; ++i)
    {
      for (u32 j = 0; j < world->tileCountY; ++j)
      {
        world->tileMap[i * world->tileCountY + j].bound = GetTileBound(world, i, j);
        world->tileMap[i * world->tileCountY + j].texture = &gameState->background;
      }
    }

    world->objCount = 2;
    world->obj = PUSH_ARRAY(&gameState->pool, rec, world->objCount);
    for (u32 i = 0; i < world->objCount; ++i)
    {
      world->obj[i].pos.x = 2.0f * (float)i;
      world->obj[i].pos.y = 2;
      world->obj[i].width = 1;
      world->obj[i].height = 1;
    }



    gameMemory->initialized = true;
  }

  game_world* world = gameState->world;
  game_camera* cam = world->cam;


  float velocity = 5.0f;

  rec* player = &gameState->player;
  player->width = 0.5f;
  player->height = 1.5f;

  v2 dir = { };
  if (input.right.isDown)
  {
    dir.x += 1.0f;
  }
  if (input.up.isDown)
  {
    dir.y += 1.0f;
  }
  if (input.down.isDown)
  {
    dir.y -= 1.0f;
  }
  if (input.left.isDown)
  {
    dir.x -= 1.0f;
  }


  MoveAndSlide(player, dir.Normalize() * velocity * input.timeStep, world);
  cam->pos = player->pos;

  RenderScreen(buffer, 0);
  DEBUGDrawBMP(buffer, gameState->background.pixel, gameState->background.width, gameState->background.height);

  u32 gray = 0x757D75;
  for (u32 i = 0; i < world->tileCountX; ++i)
  {
    for (u32 j = 0; j < world->tileCountY; ++j)
    {
      DrawTile(buffer, world->tileMap[i * world->tileCountY + j], *cam);
    }
  }

  u32 recColor = 0x00FFFF;
  for (u32 i = 0; i < world->objCount; ++i)
  {
    DrawRectangle(buffer, recColor, world->obj[i], *cam);
  }
  DrawRectangle(buffer, 0xFFFFFF, *player, *cam);
}

extern "C" GAME_OUTPUT_SOUND(GameOutputSound)
{
  game_state* gameState = (game_state*)gameMemory->permanentStorage;
}

void RenderScreen(game_offscreen_buffer* buffer, u32 color)
{
  u8* row = (u8*)buffer->memory;
  for (int y = 0; y < buffer->height; ++y)
  {
    u32* pixel = (u32*)row;
    for (int x = 0; x < buffer->width; ++x)
    {
      *pixel++ = color;
    }
    row += buffer->pitch;
  }
}


void DrawRectangle(game_offscreen_buffer* buffer, u32 color, rec r, game_camera cam)
{
  v2 minBound = WorldPointToScreen(cam, r.GetMinBound());
  v2 maxBound = WorldPointToScreen(cam, r.GetMaxBound());

  int xMin = RoundToInt(minBound.x);
  int yMin = RoundToInt(maxBound.y);
  int xMax = RoundToInt(maxBound.x);
  int yMax = RoundToInt(minBound.y);

  u32* mem = (u32*)buffer->memory;
  for (int x = xMin; x < xMax; x++)
  {
    for (int y = yMin; y < yMax; y++)
    {
      if (x >= 0 && x < buffer->width &&
          y >= 0 && y < buffer->height)
      {
        mem[y * buffer->width + x] = color;
      }
    }
  }
}
