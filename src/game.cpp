#include "game.h"
#include "jusa_math.cpp"
#include "jusa_intrinsics.h"
#include "jusa_render.cpp"
#include "jusa_random.cpp"
#include "jusa_world.cpp"
#include "jusa_physics.cpp"

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

      if (rch.isHit)
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

void LoadEntity(transient_state* tranState, entity* it)
{
  for (size_t i = 0; i < tranState->activeEntityCount; ++i)
  {
    //NOTE: 2 entities cant be in the same spots;
    if (it->pos == tranState->activeEntity[i]->pos) return;
  }

  tranState->activeEntity[tranState->activeEntityCount++] = it;
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

static void GameEditor(render_group* renderGroup, game_state* gameState, transient_state* tranState, game_input input)
{
  game_world* world = &gameState->world;
  camera* cam = &gameState->cam;
  v2 pos = ScreenPointToWorld(cam, { (float)input.mouseX, (float)input.mouseY });
  v2 tileIndex = GetTileIndex(world, pos);
  rec tileBox = GetTileBound(world, (i32)tileIndex.x, (i32)tileIndex.y);

  PushRect(renderGroup, tileBox.pos, tileBox.size, { 1.0f, 1.0f, 1.0f, 1.0f }, BRUSH_WIREFRAME);

  if (input.mouseButtonState[0])
  {
    //NOTE: Add wall
    entity it = {};
    it.pos.xy = tileBox.pos;
    it.size.xy = tileBox.size;
    it.hp = 1;
    it.type = ENTITY_WALL;
    it.canCollide = true;
    rec hb = { it.pos.xy, it.size.xy };

    for (int i = 0; i < gameState->entityCount; ++i)
    {
      rec current = { gameState->entities[i].pos.xy, gameState->entities[i].size.xy };
      if (IsRecOverlapping(hb, current))
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
      if (IsRecOverlapping(eraser, current))
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
    LoadEntity(tranState, &gameState->entities[i]);
  }
}

loaded_bitmap* AddTexture(game_state* gameState, char* fileName)
{
  //TODO: Utility function
  return 0;
}

void UpdateEntity(game_state* gameState, transient_state* tranState, entity* it, float timeStep)
{
  rec itHB = { it->pos.xy, it->size.xy };
  entity** active = tranState->activeEntity;
  size_t activeCount = tranState->activeEntityCount;
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
        if (it->owner != active[i] && it != active[i] && IsRecOverlapping(itHB, currentHB))
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
        if (it->owner != active[i] && it->owner != active[i]->owner && it != active[i] && IsRecOverlapping(itHB, currentHB))
        {
          --active[i]->hp;

          if (it->hp > 0)
          {
            --it->hp;
          }
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
    gameState->viewDistance = 0;

    AddSound(gameState, gameMemory, "test.wav");

    game_world* world = &gameState->world;
    gameState->cam.pixelPerMeter = buffer->height / 20.0f;
    gameState->cam.offSet = { buffer->width / 2.0f, buffer->height / 2.0f };

    world->tileCountX = 1024;
    world->tileCountY = 1024;

    world->tilesInChunk = { 16.0f, 16.0f };
    world->tileSizeInMeter = { 1.0f, 1.0f };

    gameState->knight = DEBUGLoadBMP(gameState, gameMemory, "knight_idle_anim_f0.bmp");
    entity player = {};
    player.size.xy = { 0.9f, 0.9f };
    player.hp = 10;
    player.type = ENTITY_PLAYER;
    player.canCollide = true;
    player.canUpdate = true;
    gameState->player = AddEntity(gameState, &player);

    LoadLevel(gameState, gameMemory, "level_demo.level");

    gameState->viewDistance = 1;

    gameState->wall = DEBUGLoadBMP(gameState, gameMemory, "wall_side_left.bmp");

    gameMemory->isInited = true;
  }

  transient_state tranState = {};
  InitMemoryArena(&tranState.tranArena, gameMemory->transientStorageSize, gameMemory->transientStorage);

  tranState.activeEntity = PUSH_ARRAY(&tranState.tranArena, entity*, MAX_ACTIVE_ENTITY);
  tranState.activeEntityCount = 0;

  ClearBuffer(buffer, { 0.0f, 0.0f, 0.0f, 0.5f });

  game_world* world = &gameState->world;
  render_group* renderGroup = InitRenderGroup(&tranState.tranArena, MEGABYTES(4), &gameState->cam);

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

    v2 currentChunk = GetChunk(gameState->cam.pos.xy, world);
    for (size_t i = 0; i < gameState->entityCount; ++i)
    {
      if (Abs(GetChunk(gameState->entities[i].pos.xy, world) - currentChunk) <= (float)gameState->viewDistance)
      {
        LoadEntity(&tranState, &gameState->entities[i]);
      }
    }

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
      v2 mousePos = ScreenPointToWorld(&gameState->cam, { (float)input.mouseX, (float)input.mouseY });
      v2 attackOffset = (mousePos - player->pos.xy).Normalize();
      //NOTE: sword position is relative to owner!
      sword.pos = V3(attackOffset, 0.0f);
      sword.size = { 1.0f, 1.0f };
      sword.owner = player;
      LoadEntity(&tranState, &sword);
    }
    if (input.mouseButtonState[1])
    {
      //TODO: Write a utility function
      entity e = {};
      entity* projectile = AddEntity(gameState, &e);
      projectile->type = ENTITY_PROJECTILE;
      v2 mousePos = ScreenPointToWorld(&gameState->cam, { (float)input.mouseX, (float)input.mouseY });
      v2 attackOffset = (mousePos - player->pos.xy).Normalize();
      //NOTE: sword position is relative to owner!
      projectile->owner = player;
      projectile->hp = 1;
      projectile->lifeTime = 4;
      projectile->size = V3(0.3f, 0.3f, 0.0f);
      projectile->pos.xy = projectile->owner->pos.xy + attackOffset;
      projectile->vel.xy = attackOffset * 5;
      LoadEntity(&tranState, projectile);
    }

    for (size_t i = 0; i < tranState.activeEntityCount; ++i)
    {
      UpdateEntity(gameState, &tranState, tranState.activeEntity[i], input.dt);
    }
    player->vel -= 0.2f * player->vel;
    gameState->cam.pos = player->pos;

  }
  else if (gameState->programMode == MODE_MENU)
  {
    if (input.escape.isDown && input.escape.halfTransitionCount == 1)
    {
      gameState->programMode = MODE_NORMAL;
    }
  }
  else if (gameState->programMode == MODE_EDITOR)
  {
    if (input.escape.isDown && input.escape.halfTransitionCount == 1)
    {
      gameState->programMode = MODE_NORMAL;

      SaveLevel(gameState, gameMemory, "level_demo.level");
    }

    GameEditor(renderGroup, gameState, &tranState, input);
  }

  for (int i = 0; i < tranState.activeEntityCount; ++i)
  {
    switch (tranState.activeEntity[i]->type)
    {
      case ENTITY_PLAYER:
      {
        PushBitmap(renderGroup, &gameState->knight, tranState.activeEntity[i]->pos.xy, tranState.activeEntity[i]->size.xy);
      } break;

      case ENTITY_WALL:
      {
        PushBitmap(renderGroup, &gameState->wall, tranState.activeEntity[i]->pos.xy, tranState.activeEntity[i]->size.xy);
      } break;

      case ENTITY_PROJECTILE:
      case ENTITY_SWORD:
      {
        entity* it = tranState.activeEntity[i];
        PushRect(renderGroup, it->pos.xy, it->size.xy, { 1.0f, 1.0f, 1.0f, 1.0f });
      } break;
    }
  }
  float angle = gameState->time;
  v3 origin = V3(0, 0, 0);
  v3 xAxis = 2*Sin(angle) * V3(Cos(angle), Sin(angle), 0);
  v3 yAxis = V3(Perp(xAxis.xy), 0);
  v3 zAxis = V3(0, 0, 0);
  CoordinateSystem(renderGroup, origin, xAxis, yAxis, zAxis, { Sin(angle), Cos(angle), 0.0f, 1.0f }, &gameState->wall);
  gameState->time += input.dt;

  RenderGroupOutput(renderGroup, buffer);

  if (input.f3.isDown && input.f3.halfTransitionCount == 1)
  {
    gameState->isMuted = !gameState->isMuted;
  }
}

void PlaySound(game_sound_output* soundBuffer, loaded_sound* sound)
{
  if (sound->sampleIndex < sound->samplesCount)
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