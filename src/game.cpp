#include "game.h"
#include "jusa_math.cpp"
#include "jusa_intrinsics.h"
#include "jusa_utils.h"

void ClearScreen(game_offscreen_buffer* buffer, color c);
void DrawRectangle(game_offscreen_buffer* buffer, color c, rec r, game_camera cam);

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


void DrawLine(game_offscreen_buffer* buffer, v2 from, v2 to, u32 lineColor)
{
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

bool RayToRec(v2 rayOrigin, v2 rayVec, rec r, v2* contactPoint, v2* contactNormal, float* tHit)
{
  if (rayVec == 0.0f) return false;
  v2 minPos = r.GetMinBound();
  v2 maxPos = r.GetMaxBound();

  v2 tNear = (minPos - rayOrigin) / rayVec;
  v2 tFar = (maxPos - rayOrigin) / rayVec;

  //Note: check if value is a floating point number!
  if (tFar != tFar || tNear != tNear) return false;

  if (tNear.x > tFar.x) Swap(&tNear.x, &tFar.x);
  if (tNear.y > tFar.y) Swap(&tNear.y, &tFar.y);

  if (tNear.x >= tFar.y || tNear.y >= tFar.x) return false;

  *tHit = Max(tNear.x, tNear.y);
  float tHitFar = Min(tFar.x, tFar.y);

  if (tHitFar <= 0 || (*tHit) >= 1.0f) return false;

  *contactPoint = rayOrigin + (rayVec * (*tHit));

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

void MoveAndSlide(entity* entity, v2 motion, game_world* world, float timeStep)
{
  //TODO: solution for world boundary
  rec* hitbox = &entity->hitbox;
  v2 dp = {};
  v2 moveBefore = {};
  v2 pos = hitbox->pos;

  do
  {
    float tMin = 1.0f;
    v2 minContactNormal = {};

    for (u32 y = 0; y < world->tileCountY; ++y)
    {
      for (u32 x = 0; x < world->tileCountX; ++x)
      {
        u32 tileIndex = y * world->tileCountX + x;
        rec imRec = { world->tileMap[tileIndex].bound.pos, world->tileMap[tileIndex].bound.size + hitbox->size };

        v2 contactNormal = {};
        v2 contactPoint = {};

        if (world->tileMap[tileIndex].type == TILE_WALL)
        {
          // resolve collision
          float tHit = 0.0f;
          if (RayToRec(pos, motion * timeStep, imRec, &contactPoint, &contactNormal, &tHit))
          {

            //Note: only true with aabb
            //TODO: solution for generic case if needed
            if ((tMin) > (tHit))
            {
              tMin = tHit;
              minContactNormal = contactNormal;
            }
          }
        }
      }
    }
    ASSERT(tMin >= 0.0f);
    v2 moveLeft = motion * (1.0f - tMin);
    moveBefore = motion * tMin;
    motion = (moveLeft - minContactNormal * Inner(minContactNormal, moveLeft));

    dp += moveBefore * timeStep;
    entity->vel = moveBefore;

    pos += moveBefore * timeStep;
  } while (motion != 0.0f);

  hitbox->pos += dp;

  entity->vel += motion * timeStep;
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
  //NOTE: x, y start at top left corner!
  rec result = {};

  result.size = world->tileSizeInMeter;

  v2 halfTileMapSizeInMeter = (world->tileSizeInMeter * V2((float)world->tileCountX, (float)world->tileCountY)) / 2.0f;

  v2 tilePos = { (float)x, (float)(world->tileCountY - y - 1) };

  result.pos = (tilePos * world->tileSizeInMeter) - halfTileMapSizeInMeter;

  return result;
}

inline img DEBUGLoadBMP(game_memory* mem, game_state* gameState, char* fileName)
{
  img result = {};

  debug_read_file_result file = mem->DEBUGPlatformReadFile(fileName);
  if (file.contentSize > 0)
  {
    bmp_header* header = (bmp_header*)file.contents;
    u32* filePixel = (u32*)((u8*)file.contents + header->offSet);

    result.pixel = PUSH_ARRAY(&gameState->pool, u32, header->width * header->height);
    result.width = header->width;
    result.height = header->height;

    bit_scan_result redScan = FindLowestSetBit(header->redMask);
    bit_scan_result greenScan = FindLowestSetBit(header->greenMask);
    bit_scan_result blueScan = FindLowestSetBit(header->blueMask);
    bit_scan_result alphaScan = FindLowestSetBit(header->alphaMask);

    ASSERT(redScan.found);
    ASSERT(blueScan.found);
    ASSERT(greenScan.found);

    i32 alphaRotate = 24 - (i32)alphaScan.index;
    i32 redRotate = 16 - (i32)redScan.index;
    i32 greenRotate = 8 - (i32)greenScan.index;
    i32 blueRotate = 0 - (i32)blueScan.index;

    for (u32 y = 0; y < header->height; ++y)
    {
      for (u32 x = 0; x < header->width; ++x)
      {
        u32 indexMem = y * header->width + x;
        u32 index = (header->height - y - 1) * header->width + x;

        u32 red = RotateLeft((filePixel[index] & header->redMask), redRotate);
        u32 green = RotateLeft((filePixel[index] & header->greenMask), greenRotate);
        u32 blue = RotateLeft((filePixel[index] & header->blueMask), blueRotate);

        u32 alpha = (alphaScan.found) ? RotateLeft((filePixel[index] & header->alphaMask), alphaRotate) : 0xFF000000;
        result.pixel[indexMem] = (alpha | red | green | blue);
      }
    }
  }
  return result;
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

static void DrawImage(game_offscreen_buffer* buffer, img* image)
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

inline v2 WorldPointToScreen(game_camera cam, v2 point)
{
  v2 relPos = point - cam.pos;
  v2 screenCoord = { relPos.x * (float)cam.pixelPerMeter, -relPos.y * (float)cam.pixelPerMeter };
  return screenCoord + cam.offSet;
}

inline void DrawTile(game_offscreen_buffer* buffer, tile tile, game_camera cam)
{
  if (tile.texture == NULL)
  {
    return;
  }
  v2 minBound = WorldPointToScreen(cam, tile.bound.GetMinBound());
  v2 maxBound = WorldPointToScreen(cam, tile.bound.GetMaxBound());

  int xMin = (int)Round(minBound.x);
  int yMin = (int)Round(maxBound.y);
  int xMax = (int)Round(maxBound.x);
  int yMax = (int)Round(minBound.y);

  ASSERT(xMax >= xMin);
  ASSERT(yMax >= yMin);

  u32* pixel = tile.texture->pixel;

  u32* scrPixel = (u32*)buffer->memory;
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

        u32 scrIndex = y * buffer->width + x;
        scrPixel[scrIndex] = LinearBlend(scrPixel[scrIndex], pixel[pY * tile.texture->width + pX]);
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

    gameState->background = DEBUGLoadBMP(gameMemory, gameState, "test.bmp");
    gameState->world = PUSH_TYPE(&gameState->pool, game_world);
    game_world* world = gameState->world;
    world->cam = PUSH_TYPE(&gameState->pool, game_camera);
    world->cam->pixelPerMeter = buffer->height / 9.0f;
    world->cam->offSet = { buffer->width / 2.0f, buffer->height / 2.0f };

    img map = DEBUGLoadBMP(gameMemory, gameState, "map.bmp");

    world->tileCountX = map.width;
    world->tileCountY = map.height;
    world->tileSizeInMeter = { 1.0f, 1.0f };
    world->tileMap = PUSH_ARRAY(&gameState->pool, tile, world->tileCountX * world->tileCountY);

    for (u32 y = 0; y < world->tileCountY; ++y)
    {
      for (u32 x = 0; x < world->tileCountX; ++x)
      {
        u32 mapIndex = y * world->tileCountX + x;
        world->tileMap[mapIndex].bound = GetTileBound(world, x, y);
        switch (map.pixel[mapIndex])
        {
          case 0xFF000000:
          {
            world->tileMap[mapIndex].type = TILE_WALL;
            world->tileMap[mapIndex].texture = &gameState->background;
          }
          break;
          case 0xFFFFFFFF:
          {
            world->tileMap[mapIndex].type = TILE_WALKABLE;
            world->tileMap[mapIndex].texture = NULL;
          }
          break;

          default:
          {
            world->tileMap[mapIndex].type = TILE_NONE;
          }
          break;
        }
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

    rec* playerHitbox = &gameState->player.hitbox;
    playerHitbox->width = 0.5f;
    playerHitbox->height = 0.5f;

    gameMemory->initialized = true;
  }

  game_world* world = gameState->world;
  game_camera* cam = world->cam;

  entity* player = &gameState->player;

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

  // float m = 1.0f;
  // float muy = 0.5f;
  // float g = 9.8f;

  v2 playerAccel = dir.Normalize() * 20.0f;

  //TODO: reset accelatation when hit wall
  playerAccel -= 4.0f * player->vel;
  v2 motion = (0.5f * playerAccel * input.timeStep) + (player->vel);

  MoveAndSlide(player, motion, world, input.timeStep);

  player->vel += playerAccel * input.timeStep;

  cam->pos = player->hitbox.pos;

  ClearScreen(buffer, { 0.0f, 0.0f, 0.0f, 0.0f });

  u32 gray = 0x757D75;
  for (u32 y = 0; y < world->tileCountY; ++y)
  {
    for (u32 x = 0; x < world->tileCountX; ++x)
    {
      u32 index = y * world->tileCountX + x;
      if (world->tileMap[index].texture == NULL)
      {
        DrawRectangle(buffer, { 0.0f, 0.0f, 0.0f, 1.0f }, world->tileMap[index].bound, *cam);
      }
      else
      {
        DrawTile(buffer, world->tileMap[index], *cam);
      }
    }
  }

  DrawRectangle(buffer, { 1.0f, 1.0f, 1.0f, 1.0f }, player->hitbox, *cam);
}

extern "C" GAME_OUTPUT_SOUND(GameOutputSound)
{
  game_state* gameState = (game_state*)gameMemory->permanentStorage;
}

void ClearScreen(game_offscreen_buffer* buffer, color c)
{
  u8* row = (u8*)buffer->memory;
  u32 col = c.ToU32();
  for (int y = 0; y < buffer->height; ++y)
  {
    u32* pixel = (u32*)row;
    for (int x = 0; x < buffer->width; ++x)
    {
      *pixel++ = col;
    }
    row += buffer->pitch;
  }
}


void DrawRectangle(game_offscreen_buffer* buffer, color c, rec r, game_camera cam)
{
  v2 minBound = WorldPointToScreen(cam, r.GetMinBound());
  v2 maxBound = WorldPointToScreen(cam, r.GetMaxBound());

  int xMin = (int)Round(minBound.x);
  int yMin = (int)Round(maxBound.y);
  int xMax = (int)Round(maxBound.x);
  int yMax = (int)Round(minBound.y);

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
