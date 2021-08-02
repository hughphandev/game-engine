#include "game.h"
#include "jusa_math.cpp"
#include "jusa_intrinsics.h"
#include "jusa_utils.h"
#include "jusa_render.cpp"
#include "jusa_random.cpp"
#include "jusa_world.cpp"

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

  if (tNear.x > tFar.y || tNear.y > tFar.x) return result;

  result.t = Max(tNear.x, tNear.y);
  float tHitFar = Min(tFar.x, tFar.y);

  if (result.t < 0.0f || (result.t) > 1.0f)
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

inline void MoveEntity(entity* it, v2 motion, float timeStep)
{
  it->pos.xy += motion * timeStep;
}

void MoveAndSlide(entity* e, v2 motion, entity* active[], size_t activeCount, float timeStep)
{
  if (!e->canUpdate || Abs(motion) < 0.0001f) return;
  if (!e->canCollide)
  {
    e->pos += e->vel * timeStep;
    return;
  }

  //TODO: solution for world boundary
  v2 orgMotion = motion;
  v2 dp = {};
  v2 moveBefore = {};
  v2 moveLeft = {};
  v2 pos = e->pos.xy;
  entity* last = 0;
  v2 minContactNormal = {};

  do
  {
    float tMin = 1.0f;

    for (i32 i = 0; i < activeCount; ++i)
    {
      if (!active[i]->canCollide || e->pos == active[i]->pos) continue;

      rec imRec = { active[i]->pos.xy,  active[i]->size.xy + e->size.xy };

      ray_cast_hit rch = RayToRec(pos, motion * timeStep, imRec);

      if (rch.isContact)
      {

        //Note: only true with aabb
        //TODO: solution for generic case if needed
        if (rch.t >= 0.0f && rch.t < tMin)
        {
          tMin = rch.t;
          minContactNormal = rch.cNormal;
          last = active[i];
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
      moveLeft = {};
    }
  } while (Abs(moveLeft) > 0.0001f);

  e->pos.xy += dp;
  e->vel.xy = (orgMotion - minContactNormal * Inner(minContactNormal, orgMotion));
}

#define PUSH_ARRAY(pool, type, count) (type*)PushSize_(pool, sizeof(type) * (count))
#define PUSH_TYPE(pool, type) (type*)PushSize_(pool, sizeof(type))
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
  v2 relPos = point - cam->pos.xy;
  v2 screenCoord = { relPos.x * (float)cam->pixelPerMeter, -relPos.y * (float)cam->pixelPerMeter };
  return screenCoord + cam->offSet.xy;
}

inline v2 ScreenPointToWorld(game_camera* cam, v2 point)
{
  //TODO: Test
  //TODO: fix offset of the pointer to the top-left of the window or change mouseX, mouseY relative to the top-left of the window
  v2 relPos = point - cam->offSet.xy;
  v2 worldCoord = { relPos.x / (float)cam->pixelPerMeter, -relPos.y / (float)cam->pixelPerMeter };
  return worldCoord + cam->pos.xy;
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
  if (texture == 0)
  {
    DrawRectangle(buffer, cam, bound, { 1.0f, 1.0f, 1.0f, 1.0f });
    return;
  }

  bound.pos += texture->anchor * bound.size;

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

void LoadEntity(game_state* gameState, memory_arena* entityArena, entity* it)
{
  size_t entityCount = entityArena->used / sizeof(it);
  entity** active = (entity**)entityArena->base;
  for (size_t i = 0; i < entityCount; ++i)
  {
    //NOTE: 2 entities cant be in the same spots;
    if (it->pos == active[i]->pos) return;
  }

  entity** e = PUSH_TYPE(entityArena, entity*);
  *e = it;
}

entity* AddEntity(game_state* gameState, entity* it)
{
  ASSERT(gameState->entityCount <= ARRAY_COUNT(gameState->entities));

  for (int i = 0; i < gameState->entityCount; ++i)
  {
    if (gameState->entities[i].size == 0)
    {
      entity* result = &gameState->entities[i];
      *result = *it;
      return result;
    }
  }

  entity* result = &gameState->entities[gameState->entityCount++];
  *result = *it;
  return result;
}

void RemoveEntity(game_state* gameState, entity* it)
{
  if (it == 0 || it < &gameState->entities[0] || it >= &gameState->entities[gameState->entityCount])
  {
    return;
  }

  if (it == &gameState->entities[gameState->entityCount - 1])
  {
    --gameState->entityCount;
  }
  else
  {
    entity* last = &gameState->entities[gameState->entityCount - 1];
    Memcpy(it, it + 1, (char*)last - (char*)it);
    --gameState->entityCount;
  }

}

void SaveLevel(game_state* gameState, game_memory* mem, char* fileName)
{
  mem->DEBUGPlatformWriteFile(fileName, gameState->entityCount * sizeof(entity), gameState->entities);
}

void LoadLevel(game_state* gameState, game_memory* mem, char* fileName)
{
  debug_read_file_result fileLevel = mem->DEBUGPlatformReadFile(fileName);
  if (fileLevel.contentSize > 0)
  {
    entity* entities = (entity*)fileLevel.contents;
    gameState->entityCount = fileLevel.contentSize / sizeof(entity);
    for (int i = 0; i < gameState->entityCount; ++i)
    {
      gameState->entities[i] = entities[i];
    }
    mem->DEBUGPlatformFreeMemory(entities);
  }
}

void AddSound(game_state* gameState, game_memory* gameMemory, loaded_sound* sound)
{
  gameState->sounds[gameState->soundCount++] = *sound;
}

void AddSound(game_state* gameState, game_memory* gameMemory, char* soundName)
{
  gameState->sounds[gameState->soundCount++] = DEBUGLoadWAV(gameState, gameMemory, soundName);
}


inline bool IsInChunk(entity* it, game_world* world, i32 chunkX, i32 chunkY)
{
  v2 chunk = GetChunk(it->pos.xy, world);
  return ((i32)chunk.x == chunkX && (i32)chunk.y == chunkY);
}

static void GameEditor(game_offscreen_buffer* buffer, game_state* gameState, game_input input)
{
  game_world* world = &gameState->world;
  game_camera* cam = &gameState->cam;
  v2 pos = ScreenPointToWorld(cam, { (float)input.mouseX, (float)input.mouseY });
  v2 tileIndex = GetTileIndex(world, pos);
  rec tileBox = GetTileBound(world, (i32)tileIndex.x, (i32)tileIndex.y);

  DrawRectangle(buffer, cam, tileBox, { 1.0f, 1.0f, 1.0f, 1.0f }, BRUSH_WIREFRAME);

  if (input.mouseButtonState[0])
  {
    //NOTE: Add wall
    entity it = {};
    it.pos.xy = tileBox.pos;
    it.size.xy = tileBox.size;
    it.vp = visible_pieces{ {gameState->textures[0]}, 1 };
    it.hp = 1;
    it.type = ENTITY_WALL;
    it.canCollide = true;
    rec hb = { it.pos.xy, it.size.xy };

    for (int i = 0; i < gameState->entityCount; ++i)
    {
      rec current = { gameState->entities[i].pos.xy, gameState->entities[i].size.xy };
      if (IsBoxOverlapping(hb, current))
      {
        break;
      }
      else if (i == gameState->entityCount - 1)
      {
        AddEntity(gameState, &it);
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
      rec current = { gameState->entities[i].pos.xy, gameState->entities[i].size.xy };
      if (IsBoxOverlapping(eraser, current))
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
  cam->pos.xy += dir.Normalize() * 10.0f * input.dt;

  for (int i = 0; i < gameState->entityCount; ++i)
  {
    rec currentHB = {gameState->entities[i].pos.xy, gameState->entities[i].size.xy};
    if (gameState->entities[i].vp.pieceCount != 0)
    {
      DrawTexture(buffer, cam, currentHB, gameState->entities[i].vp.pieces);
    }
    else
    {
      DrawRectangle(buffer, cam, currentHB, { 1.0f, 1.0f, 1.0f, 1.0f });
    }
  }
}

loaded_bitmap* AddTexture(game_state* gameState, char* fileName)
{
  //TODO: Utility function
  return 0;
}

void UpdateEntity(game_state* gameState, entity* it, entity* active[], size_t activeCount, float timeStep)
{
  rec itHB = { it->pos.xy, it->size.xy };
  switch (it->type)
  {
    case ENTITY_PLAYER:
    {
      MoveAndSlide(it, it->vel.xy, active, activeCount, timeStep);
    } break;
    case ENTITY_SWORD:
    {
      it->pos += it->owner->pos;
      for (size_t i = 0; i < activeCount; ++i)
      {
        rec currentHB = { active[i]->pos.xy, active[i]->size.xy };
        if (it->owner != active[i] && it != active[i] && IsBoxOverlapping(itHB, currentHB))
        {
          --active[i]->hp;
        }
      }
    } break;
    case ENTITY_PROJECTILE:
    {
      it->pos += it->vel * timeStep;
      it->lifeTime -= timeStep;
      for (size_t i = 0; i < activeCount; ++i)
      {
        rec currentHB = { active[i]->pos.xy, active[i]->size.xy };
        if (it->owner != active[i] && it->owner != active[i]->owner && it != active[i] && IsBoxOverlapping(itHB, currentHB))
        {
          --active[i]->hp;
        }
      }

      if (it->lifeTime <= 0)
      {
        RemoveEntity(gameState, it);
      }

    } break;

    default:
      break;
  }

  if (it->hp <= 0.0f)
  {
    RemoveEntity(gameState, it);
  }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  game_state* gameState = (game_state*)gameMemory->permanentStorage;
  if (!gameMemory->isInited)
  {
    InitMemoryArena(&gameState->arena, gameMemory->permanentStorageSize - sizeof(game_state), (u8*)gameMemory->permanentStorage + sizeof(game_state));

    gameState->programMode = MODE_NORMAL;
    gameState->cam.viewDistance = 1;

    AddSound(gameState, gameMemory, "test.wav");

    game_world* world = &gameState->world;
    gameState->cam.pixelPerMeter = buffer->height / 20.0f;
    gameState->cam.offSet = { buffer->width / 2.0f, buffer->height / 2.0f };

    world->tileCountX = 1024;
    world->tileCountY = 1024;

    world->tilesInChunk = { 16.0f, 16.0f };
    world->tileSizeInMeter = { 1.0f, 1.0f };

    gameState->textures[1] = DEBUGLoadBMP(gameState, gameMemory, "knight_idle_anim_f0.bmp");
    entity player = {};
    player.size.xy = {0.9f, 0.9f};
    player.hp = 10;
    player.type = ENTITY_PLAYER;
    player.vp.pieceCount = 1;
    player.vp.pieces[0] = gameState->textures[1];
    player.canCollide = true;
    player.canUpdate = true;
    gameState->player = AddEntity(gameState, &player);

    LoadLevel(gameState, gameMemory, "level_demo.level");

    gameState->cam.viewDistance = 1;

    gameState->textures[0] = DEBUGLoadBMP(gameState, gameMemory, "wall_side_left.bmp");

    gameMemory->isInited = true;
  }

  memory_arena entityArena = {};
  InitMemoryArena(&entityArena, gameMemory->transientStorageSize, gameMemory->transientStorage);

  ClearBuffer(buffer, { 0.0f, 0.0f, 0.0f, 0.5f });

  game_world* world = &gameState->world;
  game_camera* cam = &gameState->cam;

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

    v2 currentChunk = GetChunk(cam->pos.xy, world);
    for (size_t i = 0; i < gameState->entityCount; ++i)
    {
      if (Abs(GetChunk(gameState->entities[i].pos.xy, world) - currentChunk) <= (float)cam->viewDistance)
      {
        LoadEntity(gameState, &entityArena, &gameState->entities[i]);
      }
    }

    entity** activeEntities = (entity**)entityArena.base;
    size_t activeEntityCount = entityArena.used / sizeof(*activeEntities);

    entity* player = gameState->player;

    v2 dir = {};

    if (input.left.isDown)
    {
      dir.x -= 1;
    }
    if (input.right.isDown)
    {
      dir.x += 1;
    }
    if (input.up.isDown)
    {
      dir.y += 1;
    }
    if (input.down.isDown)
    {
      dir.y -= 1;
    }

    v2 playerAccel = dir.Normalize() * 50.0f;

    if (input.space.isDown && input.space.halfTransitionCount == 1)
    {
      player->vel = player->vel.Normalize() * 30.0f;
    }
    else
    {
      player->vel.xy += playerAccel * input.dt;
    }


    entity sword = {};
    if (input.mouseButtonState[0])
    {
      //TODO: Write a utility function
      sword.type = ENTITY_SWORD;
      v2 mousePos = ScreenPointToWorld(cam, { (float)input.mouseX, (float)input.mouseY });
      v2 attackOffset = (mousePos - player->pos.xy).Normalize();
      //NOTE: sword position is relative to owner!
      sword.pos = V3(attackOffset, 0.0f);
      sword.size = { 1.0f, 1.0f };
      sword.owner = player;
      LoadEntity(gameState, &entityArena, &sword);
      activeEntityCount = entityArena.used / sizeof(entity*);
    }
    if (input.mouseButtonState[1])
    {
      //TODO: Write a utility function
      entity e = {};
      entity* projectile = AddEntity(gameState, &e);
      projectile->type = ENTITY_PROJECTILE;
      v2 mousePos = ScreenPointToWorld(cam, { (float)input.mouseX, (float)input.mouseY });
      v2 attackOffset = (mousePos - player->pos.xy).Normalize();
      //NOTE: sword position is relative to owner!
      projectile->owner = player;
      projectile->hp = 1;
      projectile->lifeTime = 4;
      projectile->size = V3(0.3f, 0.3f, 0.0f);
      projectile->pos.xy = projectile->owner->pos.xy + attackOffset;
      projectile->vel.xy = attackOffset * 5;
      LoadEntity(gameState, &entityArena, projectile);
      activeEntityCount = entityArena.used / sizeof(entity*);
    }

    for (int i = 0; i < activeEntityCount; ++i)
    {
      UpdateEntity(gameState, activeEntities[i], activeEntities, activeEntityCount, input.dt);
    }
    player->vel -= 0.2f * player->vel;
    cam->pos = player->pos;

    for (int i = 0; i < activeEntityCount;++i)
    {
      rec current = { activeEntities[i]->pos.xy, activeEntities[i]->size.xy };
      if (activeEntities[i]->vp.pieceCount != 0)
      {
        DrawTexture(buffer, cam, current, activeEntities[i]->vp.pieces);
      }
      else
      {
        DrawRectangle(buffer, cam, current, { 1.0f, 1.0f, 1.0f, 1.0f });
      }
    }
  }
  else if (gameState->programMode == MODE_MENU)
  {
    if (input.escape.isDown && input.escape.halfTransitionCount == 1)
    {
      gameState->programMode = MODE_NORMAL;
    }
    DrawImage(buffer, &gameState->textures[1]);
  }
  else if (gameState->programMode == MODE_EDITOR)
  {
    if (input.escape.isDown && input.escape.halfTransitionCount == 1)
    {
      gameState->programMode = MODE_NORMAL;

      SaveLevel(gameState, gameMemory, "level_demo.level");
    }

    GameEditor(buffer, gameState, input);
  }


  if (input.f3.isDown && input.f3.halfTransitionCount == 1)
  {
    gameState->isMuted = !gameState->isMuted;
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

  if (!gameState->isMuted)
  {
    PlaySound(soundBuffer, &gameState->sounds[0]);
  }
}