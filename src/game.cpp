#include "game.h"
#include "jusa_math.cpp"
#include "jusa_intrinsics.h"
#include "jusa_utils.h"
#include "jusa_render.h"
#include <stdlib.h>
#include "jusa_random.h"


v2 rec::GetMinBound()
{
  //TODO: Test
  return { this->pos.x - (this->width / 2.0f), this->pos.y - (this->height / 2.0f) };
}

v2 rec::GetMaxBound()
{
  //TODO: Test
  return { this->pos.x + (this->width / 2.0f), this->pos.y + (this->height / 2.0f) };
}


bool IsBoxOverlapping(rec a, rec b)
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

struct ray_cast_hit
{
  bool isContact;
  float t;
  v2 cPoint;
  v2 cNormal;
};

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

  if (tNear.x >= tFar.y || tNear.y >= tFar.x) return result;

  result.t = Max(tNear.x, tNear.y);
  float tHitFar = Min(tFar.x, tFar.y);

  if (tHitFar <= 0 || (result.t) >= 1.0f)
  {
    result.isContact = false;
  }
  else
  {
    result.isContact = true;
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

void MoveAndSlide(entity* e, v2 motion, game_state* gameState, float timeStep)
{
  if (Abs(motion) < 0.0001f) return;

  //TODO: solution for world boundary
  v2 orgMotion = motion;
  rec* hitbox = &e->hitbox;
  v2 dp = {};
  v2 moveBefore = {};
  v2 moveLeft = {};
  v2 pos = hitbox->pos;
  game_world* world = &gameState->world;
  entity* last = NULL;
  v2 minContactNormal = {};

  do
  {
    float tMin = 1.0f;

    for (i32 i = 0; i < gameState->entityCount; ++i)
    {
      rec current = gameState->entities[i].hitbox;
      if (e->hitbox.pos == current.pos) continue;

      rec imRec = { current.pos,  current.size + e->hitbox.size };

      ray_cast_hit rch = RayToRec(pos, motion * timeStep, imRec);

      if (rch.isContact)
      {

        //Note: only true with aabb
        //TODO: solution for generic case if needed
        if (rch.t >= 0.0f && rch.t < tMin)
        {
          tMin = rch.t;
          minContactNormal = rch.cNormal;
          last = &gameState->entities[i];
        }
      }
    }

    if (tMin >= 0.0f)
    {
      moveLeft = motion * (1.0f - tMin);
      moveBefore = motion * tMin;
      motion = (moveLeft - minContactNormal * Inner(minContactNormal, moveLeft));

      dp += moveBefore * timeStep;

      pos += moveBefore * timeStep;
    }
    else
    {
      moveBefore = motion;
      dp = moveBefore * timeStep;
      moveLeft = 0.0f;
    }
  } while (Abs(moveLeft) > 0.0001f);

  hitbox->pos += dp;
  e->vel = (orgMotion - minContactNormal * Inner(minContactNormal, orgMotion));
}

#define PUSH_ARRAY(pool, type, count) (type*)PushSize_(pool, sizeof(type) * (count))
#define PUSH_TYPE(pool, type) (type*)PushSize_(pool, sizeof((type))
static void* PushSize_(memory_arena* pool, size_t bytes)
{
  ASSERT((pool->size - pool->used) >= bytes);

  void* result = (u8*)pool->base + pool->used;
  pool->used += bytes;
  return result;
}

static void InitMemoryArena(memory_arena* arena, size_t size, void* base)
{
  arena->base = (u8*)base;
  arena->size = size;
  arena->used = 0;
}

inline rec GetTileBound(game_world* world, i32 x, i32 y)
{
  //NOTE: x, y start at top left corner!
  //TODO: Test
  rec result = {};

  result.size = world->tileSizeInMeter;

  v2 halfTileMapSizeInMeter = V2((float)world->tileCountX, (float)world->tileCountY) / 2.0f;

  v2 tilePos = { (float)x, (float)(world->tileCountY - y - 1) };

  result.pos = (tilePos - halfTileMapSizeInMeter) * world->tileSizeInMeter;

  return result;
}

inline v2 GetTileIndex(game_world* world, float x, float y)
{
  //TODO: Test
  v2 halfTileCount = Round(0.5f * V2((float)world->tileCountX, (float)world->tileCountY));
  return Round(V2(x, -y - 1) / world->tileSizeInMeter) + halfTileCount;
}

inline v2 GetTileIndex(game_world* world, v2 pos)
{
  return GetTileIndex(world, pos.x, pos.y);
}

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24)) 
enum
{
  WAVE_CHUNKID_FMT = RIFF_CODE('f', 'm', 't', ' '),
  WAVE_CHUNKID_DATA = RIFF_CODE('d', 'a', 't', 'a'),
  WAVE_CHUNKID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
  WAVE_CHUNKID_RIFF = RIFF_CODE('R', 'I', 'F', 'F')
};

riff_chunk* NextChunk(riff_chunk* chunk)
{
  u8* data = (u8*)(chunk + 1);
  return (riff_chunk*)(data + chunk->dataSize);
}

bool Isvalid(debug_read_file_result file, void* mem)
{
  return mem < ((u8*)file.contents + file.contentSize);
}

void* GetChunkData(riff_chunk* chunk)
{
  return chunk + 1;
}

inline loaded_sound DEBUGLoadWAV(game_state* gameState, game_memory* mem, char* fileName)
{
  loaded_sound result = {};
  debug_read_file_result file = mem->DEBUGPlatformReadFile(fileName);
  if (file.contentSize > 0)
  {
    wav_header* header = (wav_header*)file.contents;
    ASSERT(header->waveID == WAVE_CHUNKID_WAVE);
    ASSERT(header->riffID == WAVE_CHUNKID_RIFF);

    size_t dataSize = 0;
    size_t nChannels = 0;
    size_t bytesPerSample = 0;
    void* data = 0;

    for (riff_chunk* chunk = (riff_chunk*)(header + 1); Isvalid(file, chunk); chunk = NextChunk(chunk))
    {
      switch (chunk->id)
      {
        case WAVE_CHUNKID_FMT:
        {
          wav_fmt* fmt = (wav_fmt*)GetChunkData(chunk);
          nChannels = fmt->nChannels;
          bytesPerSample = fmt->blockAlign / fmt->nChannels;

          ASSERT(fmt->audioFormat == 1); //NOTE: only support PCM
          ASSERT(bytesPerSample == 2); //TODO: onpy 16 bit samples for now!
        } break;
        case WAVE_CHUNKID_DATA:
        {
          data = GetChunkData(chunk);
          dataSize = chunk->dataSize;
        } break;
      }
    }

    result.mem = data;
    result.samplesCount = dataSize / bytesPerSample;
    result.channelsCount = nChannels;
  }
  return result;
};

inline loaded_bitmap DEBUGLoadBMP(game_state* gameState, game_memory* mem, char* fileName)
{
  loaded_bitmap result = {};

  debug_read_file_result file = mem->DEBUGPlatformReadFile(fileName);
  if (file.contentSize > 0)
  {
    bmp_header* header = (bmp_header*)file.contents;
    u32* filePixel = (u32*)((u8*)file.contents + header->offSet);

    result.pixel = PUSH_ARRAY(&gameState->arena, u32, header->width * header->height);
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
    mem->DEBUGPlatformFreeMemory(file.contents);
  }
  return result;
}

inline v2 WorldPointToScreen(game_camera* cam, v2 point)
{
  //TODO: Test
  v2 relPos = point - cam->pos;
  v2 screenCoord = { relPos.x * (float)cam->pixelPerMeter, -relPos.y * (float)cam->pixelPerMeter };
  return screenCoord + cam->offSet;
}

inline v2 ScreenPointToWorld(game_camera* cam, v2 point)
{
  //TODO: Test
  //TODO: fix offset of the pointer to the top-left of the window or change mouseX, mouseY relative to the top-left of the window
  v2 relPos = point - cam->offSet;
  v2 worldCoord = { relPos.x / (float)cam->pixelPerMeter, -relPos.y / (float)cam->pixelPerMeter };
  return worldCoord + cam->pos;
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

enum brush_type
{
  BRUSH_FILL,
  BRUSH_WIREFRAME
};

void DrawRectangle(game_offscreen_buffer* buffer, game_camera* cam, rec r, color c, brush_type brush = BRUSH_FILL)
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

inline void DrawTexture(game_offscreen_buffer* buffer, game_camera* cam, rec bound, loaded_bitmap* texture)
{
  if (texture == NULL)
  {
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

entity* AddEntity(game_state* gameState, entity it)
{
  ASSERT(gameState->entityCount <= ARRAY_COUNT(gameState->entities));

  entity* result = &gameState->entities[gameState->entityCount++];
  *result = it;
  return result;
}

void RemoveEntity(game_state* gameState, entity* it)
{
  ASSERT(it != NULL);

  size_t entityIndex = it - gameState->entities;
  size_t copySize = gameState->entityCount - entityIndex - 1;
  entity* next = it + 1;
  if (copySize > 0)
  {
    Memcpy(it, next, copySize * sizeof(*it));
  }
  --gameState->entityCount;
}

struct level
{
  i32 entityCount;
  entity* entities;
};

void SaveLevel(game_state* gameState, char* fileName, debug_platform_write_file WriteFile)
{
  WriteFile(fileName, gameState->entityCount * sizeof(entity), gameState->entities);
}

void LoadLevel(game_state* gameState, char* fileName, debug_platform_read_file ReadFile, debug_platform_free_memory FreeMemory)
{
  debug_read_file_result fileLevel = ReadFile(fileName);
  if (fileLevel.contentSize > 0)
  {
    entity* entities = (entity*)fileLevel.contents;
    gameState->entityCount = fileLevel.contentSize / sizeof(entity);
    for (int i = 0; i < gameState->entityCount; ++i)
    {
      gameState->entities[i] = entities[i];
    }
    FreeMemory(entities);
  }
}

void AddSound(game_state* gameState, game_memory* gameMemory, loaded_sound sound)
{
  gameState->sounds[gameState->soundCount++] = sound;
}

void AddSound(game_state* gameState, game_memory* gameMemory, char* soundName)
{
  gameState->sounds[gameState->soundCount++] = DEBUGLoadWAV(gameState, gameMemory, soundName);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  game_state* gameState = (game_state*)gameMemory->permanentStorage;
  if (!gameMemory->isInited)
  {
    InitMemoryArena(&gameState->arena, gameMemory->permanentStorageSize - sizeof(game_state), (u8*)gameMemory->permanentStorage + sizeof(game_state));

    gameState->programMode = MODE_NORMAL;
    // AddSound(gameState, gameMemory, "test.wav");

    AddSound(gameState, gameMemory, "test.wav");

    game_world* world = &gameState->world;
    world->cam.pixelPerMeter = buffer->height / 20.0f;
    world->cam.offSet = { buffer->width / 2.0f, buffer->height / 2.0f };

    gameState->background = DEBUGLoadBMP(gameState, gameMemory, "test.bmp");

    world->tileCountX = 512;
    world->tileCountY = 512;
    world->tileSizeInMeter = { 1.0f, 1.0f };

    entity player = { 0.0f, 0.0f, 0.5f, 0.5f };
    AddEntity(gameState, player);
    gameState->playerIndex = gameState->entityCount - 1;

    LoadLevel(gameState, "level_demo.level", gameMemory->DEBUGPlatformReadFile, gameMemory->DEBUGPlatformFreeMemory);

    gameMemory->isInited = true;
  }

  ClearBuffer(buffer, { 0.0f, 0.0f, 0.0f, 0.5f });

  game_world* world = &gameState->world;
  game_camera* cam = &world->cam;

  //TODO: using if-else for now! change to state machine if needed.
  if (gameState->programMode == MODE_NORMAL)
  {
    if (input.escape.isDown && input.escape.halfTransitionCount == 1)
    {
      gameState->programMode = MODE_MENU;
    }
    if (input.f1.isDown && input.f1.halfTransitionCount == 1)
    {
      gameState->programMode = MODE_EDITOR;
    }

    entity* player = &gameState->entities[gameState->playerIndex];

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
    v2 playerAccel = dir.Normalize() * 50.0f;

    if (input.space.isDown && input.space.halfTransitionCount == 1)
    {
      player->vel = player->vel.Normalize() * 30.0f;
    }
    else
    {
      player->vel += playerAccel * input.dt;
    }

    for (int i = 0; i < gameState->entityCount; ++i)
    {
      MoveAndSlide(&gameState->entities[i], gameState->entities[i].vel, gameState, input.dt);
    }
    player->vel -= 0.2f * player->vel;

    for (int i = 0; i < gameState->entityCount; ++i)
    {
      DrawRectangle(buffer, cam, gameState->entities[i].hitbox, { 1.0f, 1.0f, 1.0f, 1.0f });
    }
  }
  else if (gameState->programMode == MODE_MENU)
  {
    if (input.escape.isDown && input.escape.halfTransitionCount == 1)
    {
      gameState->programMode = MODE_NORMAL;
    }
    DrawImage(buffer, &gameState->background);
  }
  else if (gameState->programMode == MODE_EDITOR)
  {
    if (input.escape.isDown && input.escape.halfTransitionCount == 1)
    {
      gameState->programMode = MODE_NORMAL;

      SaveLevel(gameState, "level_demo.level", gameMemory->DEBUGPlatformWriteFile);
    }
    v2 pos = ScreenPointToWorld(cam, { (float)input.mouseX, (float)input.mouseY });
    v2 tileIndex = GetTileIndex(world, pos);
    rec tileBox = GetTileBound(world, (i32)tileIndex.x, (i32)tileIndex.y);

    DrawRectangle(buffer, cam, tileBox, { 1.0f, 1.0f, 1.0f, 1.0f }, BRUSH_WIREFRAME);

    if (input.mouseButtonState[0])
    {
      entity it = {};
      it.hitbox = tileBox;

      for (int i = 0; i < gameState->entityCount; ++i)
      {
        if (IsBoxOverlapping(it.hitbox, gameState->entities[i].hitbox))
        {
          break;
        }
        else if (i == gameState->entityCount - 1)
        {
          AddEntity(gameState, it);
        }
      }
    }
    else if (input.mouseButtonState[1])
    {
      rec eraser = {};
      eraser.size = { 0.5f, 0.5f };
      eraser.pos = ScreenPointToWorld(cam, { (float)input.mouseX, (float)input.mouseY });
      for (int i = 0; i < gameState->entityCount; ++i)
      {
        if (IsBoxOverlapping(eraser, gameState->entities[i].hitbox))
        {
          RemoveEntity(gameState, &gameState->entities[i]);
        }
      }
    }

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
    cam->pos += dir.Normalize() * 10.0f * input.dt;

    for (int i = 0; i < gameState->entityCount; ++i)
    {
      DrawRectangle(buffer, cam, gameState->entities[i].hitbox, { 1.0f, 1.0f, 1.0f, 1.0f });
    }
  }
}

void PlaySound(game_sound_output* soundBuffer, loaded_sound* sound)
{
  if (sound->isLooped || sound->sampleIndex < sound->samplesCount)
  {
    i16* samples = (i16*)sound->mem + sound->sampleIndex;

    for (size_t i = 0; i < soundBuffer->sampleCount; ++i)
    {
      if (sound->channelsCount == 1)
      {
        soundBuffer->samples[i] = *(samples + i);
        soundBuffer->samples[i + 1] = *(samples + i);
        ++i;
      }
      else if (sound->channelsCount == 2)
      {
        soundBuffer->samples[i] = *(samples + i);
      }
      else
      {
        ASSERT(!"Currently not support more than 2 channels")
      }
    }
    sound->sampleIndex += soundBuffer->sampleCount * (2 / sound->channelsCount);
  }
}

void MixSound(game_sound_output* soundBuffer, loaded_sound* sounds, size_t soundCount)
{
  //TODO: Sound mixer
}

extern "C" GAME_OUTPUT_SOUND(GameOutputSound)
{
  game_state* gameState = (game_state*)gameMemory->permanentStorage;

  PlaySound(soundBuffer, &gameState->sounds[0]);
}