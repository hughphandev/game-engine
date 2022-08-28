#ifndef GAME_CPP
#define GAME_CPP

#include "game.h"
#include "intrinsics.h"
#include "math.cpp"
#include "render.cpp"
#include "random.cpp"
#include "world.cpp"
#include "physics.cpp"
#include "assets.cpp"

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
      motion = (moveLeft - minContactNormal * Dot(minContactNormal, moveLeft));

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
  e->vel.xy = (orgMotion - minContactNormal * Dot(minContactNormal, orgMotion));
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
  mem->fileIO.Write(fileName, gameState->entityCount * sizeof(entity), gameState->entities);
}

void LoadLevel(game_state* gameState, game_memory* mem, char* fileName)
{
  debug_read_file_result fileLevel = mem->fileIO.Read(fileName);
  if (fileLevel.contentSize > 0)
  {
    entity* entities = (entity*)fileLevel.contents;
    gameState->entityCount = fileLevel.contentSize / sizeof(entity);
    for (int i = 0; i < gameState->entityCount; ++i)
    {
      gameState->entities[i] = entities[i];
    }
    mem->fileIO.Free(entities);
  }
}

void AddSound(game_state* gameState, game_memory* gameMemory, loaded_sound* sound)
{
  gameState->sounds[gameState->soundCount++] = *sound;
}

void AddSound(game_state* gameState, game_memory* gameMemory, char* soundName)
{
  gameState->sounds[gameState->soundCount++] = DEBUGLoadWAV(&gameState->arena, gameMemory->fileIO, soundName);
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

  // PushRectOutline(renderGroup, V3(tileBox.pos, 0), tileBox.size, 1, { 1.0f, 1.0f, 1.0f, 1.0f });


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

#if INTERNAL
game_memory* g_memory;
#endif

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
#if INTERNAL
  g_memory = memory;
#endif
  BEGIN_TIMER_BLOCK(GameUpdateAndRender);

  game_state* gameState = (game_state*)memory->permanentStorage;
  if (!memory->isInit)
  {
    InitMemoryArena(&gameState->arena, memory->permanentStorageSize - sizeof(game_state), (u8*)memory->permanentStorage + sizeof(game_state));

    //NOTE: Init work queue function
    PlatformAddWorkEntry = memory->PlatformAddWorkEntry;
    PlatformCompleteAllWork = memory->PlatformCompleteAllWork;

    gameState->programMode = MODE_NORMAL;
    gameState->viewDistance = 0;

    AddSound(gameState, memory, "test.wav");

    game_world* world = &gameState->world;
    gameState->cam.meterToPixel = buffer->height / 20.0f;
    gameState->cam.offSet = { buffer->width / 2.0f, buffer->height / 2.0f };


    gameState->cam.size = V2(buffer->width, buffer->height);
    gameState->cam.rFov = 0.5f * PI;
    gameState->cam.zNear = 0.5;
    gameState->cam.zFar = 100;
    gameState->cam.pos = V3(0, 0, -5);
    gameState->cam.rot = {};

    world->tileCountX = 1024;
    world->tileCountY = 1024;

    world->tilesInChunk = { 16.0f, 16.0f };
    world->tileSizeInMeter = { 1.0f, 1.0f };

    entity player = {};
    player.size.xy = { 0.9f, 0.9f };
    player.hp = 10;
    player.type = ENTITY_PLAYER;
    player.canCollide = true;
    player.canUpdate = true;
    gameState->player = AddEntity(gameState, &player);

    LoadLevel(gameState, memory, "level_demo.level");

    gameState->viewDistance = 1;

    gameState->wall = LoadImageToArena(&gameState->arena, memory->fileIO, "wall_side_left.bmp");
    gameState->knight = LoadImageToArena(&gameState->arena, memory->fileIO, "knight_idle_anim_f0.bmp");

    // gameState->cube = DEBUGLoadObj(gameState, memory, "cube.obj");

    gameState->bricks = LoadImageToArena(&gameState->arena, memory->fileIO, "bricks.bmp");
    gameState->bricksNormal = MakeEmptyBitmap(&gameState->arena, gameState->bricks.width, gameState->bricks.height, false);
    MakeSphereNormalMap(&gameState->bricksNormal, 0.0f);

    gameState->testDiffuse = MakeEmptyBitmap(&gameState->arena, 128, 128);
    ClearBuffer(&gameState->testDiffuse, V4(0.5f, 0.5f, 0.5f, 1.0f).ToU32());
    gameState->testNormal = MakeEmptyBitmap(&gameState->arena, gameState->testDiffuse.width, gameState->testDiffuse.height, false);
    MakeSphereNormalMap(&gameState->testNormal, 0.0f);
    MakeSphereDiffuseMap(&gameState->testDiffuse);

    memory->isInit = true;
  }


  //NOTE: init transient state
  transient_state tranState = {};
  InitMemoryArena(&tranState.tranArena, memory->transientStorageSize, memory->transientStorage);
  if (!tranState.isInit)
  {
    tranState.activeEntity = PUSH_ARRAY(&tranState.tranArena, entity*, MAX_ACTIVE_ENTITY);
    tranState.activeEntityCount = 0;

    tranState.envMapWidth = 512;
    tranState.envMapHeight = 256;
    for (int mapIndex = 0; mapIndex < ARRAY_COUNT(tranState.envMap); ++mapIndex)
    {
      environment_map* map = &tranState.envMap[mapIndex];
      u32 width = tranState.envMapWidth;
      u32 height = tranState.envMapHeight;
      for (int LODIndex = 0; LODIndex < ARRAY_COUNT(map->LOD); ++LODIndex)
      {
        map->LOD[LODIndex] = MakeEmptyBitmap(&tranState.tranArena, width, height);
        width >>= 1;
        height >>= 1;
      }
    }

    tranState.envMap[0].pZ = -4.0f;
    tranState.envMap[1].pZ = 0.0f;
    tranState.envMap[2].pZ = 4.0f;

    tranState.isInit = true;
  }
  loaded_bitmap drawBuffer = {};
  drawBuffer.pixel = (u32*)buffer->memory;
  drawBuffer.width = buffer->width;
  drawBuffer.height = buffer->height;

  game_world* world = &gameState->world;
  render_group* renderGroup = InitRenderGroup(&tranState.tranArena, MEGABYTES(4), &gameState->cam);
  Clear(renderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

  // LoadEntity(&tranState, gameState->player);

  for (int i = 0; i < tranState.activeEntityCount; ++i)
  {
    switch (tranState.activeEntity[i]->type)
    {
      case ENTITY_PLAYER:
      {
        PushBitmap(renderGroup, &gameState->knight, tranState.activeEntity[i]->pos, tranState.activeEntity[i]->size.xy);
      } break;

      case ENTITY_WALL:
      {
        PushBitmap(renderGroup, &gameState->wall, tranState.activeEntity[i]->pos, tranState.activeEntity[i]->size.xy);
      } break;

      case ENTITY_PROJECTILE:
      case ENTITY_SWORD:
      {
        entity* it = tranState.activeEntity[i];
        // PushRect(renderGroup, it->pos, it->size.xy, { 1.0f, 1.0f, 1.0f, 1.0f });
      } break;
    }
  }

  v2 checkerSize = V2(16, 16);
  v3 mapCol[] = { V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1) };
  for (i32 mapIndex = 0; mapIndex < ARRAY_COUNT(tranState.envMap); ++mapIndex)
  {
    bool rowCheckerOn = false;
    environment_map* map = &tranState.envMap[mapIndex];
    for (u32 y = 0; y < tranState.envMapHeight; y += (u32)checkerSize.y)
    {
      bool checkerOn = rowCheckerOn;
      for (u32 x = 0; x < tranState.envMapWidth; x += (u32)checkerSize.x)
      {
        v3 color = checkerOn ? mapCol[mapIndex] : V3(0, 0, 0);
        v2 minP = V2((float)x, (float)y);
        v2 maxP = minP + checkerSize;
        DrawRectangle(&map->LOD[0], minP, maxP, V4(color, 1.0f));

        checkerOn = !checkerOn;
      }
      rowCheckerOn = !rowCheckerOn;
    }
  }

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

  gameState->cam.rot += V2(input.dMouseX, input.dMouseY);
  float pitch = gameState->cam.rot.y * input.dt * 0.01f;
  float yaw = (PI * 0.5f) + gameState->cam.rot.x * input.dt * 0.01f;

  renderGroup->cam->dir.x = -Cos(yaw) * Cos(pitch);
  renderGroup->cam->dir.y = Sin(pitch);
  renderGroup->cam->dir.z = Sin(yaw) * Cos(pitch);
  renderGroup->cam->dir = Normalize(renderGroup->cam->dir);

  renderGroup->cam->pos += Normalize(dir.y * renderGroup->cam->dir + dir.x * Cross(V3(0, 1, 0), renderGroup->cam->dir)) * input.dt;


  vertex ver[] = {
    { -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
    { 0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
    { 0.5f, 0.5f, -0.5f, 1.0f, 0.0f },
    { -0.5f, 0.5f, -0.5f, 0.0f, 0.0f },

    { -0.5f, -0.5f, 0.5f, 1.0f, 1.0f },
    { 0.5f, -0.5f, 0.5f, 0.0f, 1.0f },
    { 0.5f, 0.5f, 0.5f, 0.0f, 0.0f },
    { -0.5f, 0.5f, 0.5f, 1.0f, 0.0f },
  };

  i32 index[] = {
    0, 1, 2, 2, 3, 0,
    4, 7, 6, 6, 5, 4,
    4, 0, 3, 3, 7, 4,
    5, 6, 2, 2, 1, 5,
    4, 5, 1, 1, 0, 4,
    3, 2, 6, 6, 7, 3,
  };

  // for (i32 i = 0; i < ARRAY_COUNT(ver); ++i)
  // {
  //   ver[i].pos = Rotate(ver[i].pos, V3(gameState->time, -gameState->time, 0.0f));
  // }

  v4 col = V4(1.0f, 1.0f, 1.0f, 1.0f);
  directional_light sunLight = {};
  sunLight.dir = { 0.0f, -1.0f, 0.0f };
  sunLight.diffuse = { 1.0f, 1.0f, 1.0f };
  sunLight.intensity = 2.0f;
  sunLight.ambient = 0.1f * sunLight.diffuse;

  point_light pointLight = {};
  pointLight.pos = { 0.0f, 0.0f, -2.0f };
  pointLight.diffuse = { 1.0f, 1.0f, 1.0f };
  pointLight.attenuation = { 1.0f, 0.0f, 0.0f };
  pointLight.intensity = 2.0f;
  light_config light = {};
  light.pointLights = &pointLight;
  light.pointLightsCount = 1;
  // light.directionalLights = &sunLight;
  // light.directionalLightsCount = 1;
  loaded_model cube = DEBUGLoadObj(&tranState.tranArena, memory->fileIO, "cube.obj");

  for (u32 i = 0; i < cube.posCount; ++i)
  {
    cube.positions[i] = Rotate(cube.positions[i], V3(gameState->time, -gameState->time, 0.0f));
  }

  for (u32 i = 0; i < cube.norCount; ++i)
  {
    cube.normals[i] = Rotate(cube.normals[i], V3(gameState->time, -gameState->time, 0.0f));
  }

  RenderModel(renderGroup, light, &cube, col, &gameState->bricks);

  gameState->time += input.dt;

  RenderGroupOutput(memory->workQueue, &tranState.tranArena, renderGroup, &drawBuffer);

  if (input.f3.isDown && input.f3.halfTransitionCount == 1)
  {
    gameState->isMuted = !gameState->isMuted;
  }

  END_TIMER_BLOCK(GameUpdateAndRender);
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
  game_state* gameState = (game_state*)memory->permanentStorage;

  // PlaySound(soundBuffer, &gameState->sounds[0]);
}

#endif