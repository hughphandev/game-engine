#ifndef GAME_CPP
#define GAME_CPP

#include "game.h"
#include "intrinsics.h"
#include "math.cpp"
#include "render.cpp"
#include "random.cpp"
#include "world.cpp"
#include "physics.cpp"

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
}

inline char* ReadUntil(memory_arena* arena, char* input, char terminator)
{
  char* end = input;
  for (; *end != terminator; ++end);
  size_t count = end - input;
  char* result = PUSH_ARRAY(arena, char, count + 1);
  Memcpy(result, input, count);
  result[count] = '\0';
  return result;
}

inline size_t CountSubString(char* input, char* subString)
{
  size_t result = 0;
  size_t strLen = strlen(subString);
  for (char* c = input; *c != '\0'; ++c)
  {
    if (strncmp(c, subString, strLen) == 0)
    {
      ++result;
      c += strLen - 1;
    }
  }
  return result;
}

inline char* Skip(char* c, char skip)
{
  while (*c == skip) ++c;
  return c;
}
inline char* SkipUntil(char* c, char until)
{
  while (*c != until) ++c;
  return c;
}

#define MTL_NEW "newmtl"
#define MTL_USE "usemtl"
#define MTL_LIB "mtllib"
#define MTL_NS "Ns"
#define MTL_KA "Ka"
#define MTL_KD "Kd"
#define MTL_KE "Ke"
#define MTL_KS "Ks"
#define MTL_NI "Ni"
#define MTL_D "d"
#define MTL_ILLUM "illum"

#define MTL_MAP_KA "map_Ka"
#define MTL_MAP_KD "map_Kd"
#define MTL_MAP_KS "map_Ks"
#define MTL_MAP_NS "map_Ns"
#define MTL_MAP_D "map_d"
#define MTL_MAP_BUMP "bump"
#define MTL_MAP_DISP "disp"
#define MTL_MAP_DECAL "decal"

inline loaded_mtl DEBUGLoadMTL(memory_arena* arena, game_memory* mem, char* fileName)
{
  loaded_mtl result = {};
  //TODO: load mtl file
  debug_read_file_result file = mem->DEBUGPlatformReadFile(fileName);
  if (file.contentSize > 0)
  {
    char* fileEnd = (char*)file.contents + file.contentSize;
    result.matCount = CountSubString((char*)file.contents, MTL_NEW);
    result.materials = (loaded_material*)PUSH_ARRAY(arena, loaded_material, result.matCount);
    u32 matIndex = 0;
    loaded_material* mat = NULL;
    for (char* c = (char*)file.contents; c < fileEnd; ++c)
    {
      if (*c == '#') c = SkipUntil(c, '\n');
      if (strncmp(c, MTL_NEW, strlen(MTL_NEW)) == 0)
      {
        mat = &result.materials[matIndex++];
        c += strlen(MTL_NEW);
        c = Skip(c, ' ');
        mat->name = ReadUntil(arena, c, '\n');
      }
      if (strncmp(c, MTL_NS, strlen(MTL_NS)) == 0)
      {
        c += strlen(MTL_NS);
        mat->specularExponent = strtof(c, &c);
      }
      if (strncmp(c, MTL_KA, strlen(MTL_KA)) == 0)
      {
        c += strlen(MTL_KA);
        mat->ambient.x = strtof(c, &c);
        mat->ambient.y = strtof(c, &c);
        mat->ambient.z = strtof(c, &c);
      }
      if (strncmp(c, MTL_KD, strlen(MTL_KD)) == 0)
      {
        c += strlen(MTL_KD);
        mat->diffuse.x = strtof(c, &c);
        mat->diffuse.y = strtof(c, &c);
        mat->diffuse.z = strtof(c, &c);
      }
      if (strncmp(c, MTL_KS, strlen(MTL_KS)) == 0)
      {
        c += strlen(MTL_KS);
        mat->specular.x = strtof(c, &c);
        mat->specular.y = strtof(c, &c);
        mat->specular.z = strtof(c, &c);
      }
      if (strncmp(c, MTL_KE, strlen(MTL_KE)) == 0)
      {
        c += strlen(MTL_KE);
        mat->emissive.x = strtof(c, &c);
        mat->emissive.y = strtof(c, &c);
        mat->emissive.z = strtof(c, &c);
      }
      if (strncmp(c, MTL_NI, strlen(MTL_NI)) == 0)
      {
        c += strlen(MTL_NI);
        mat->opticalDensity = strtof(c, &c);
      }
      if (strncmp(c, MTL_D, strlen(MTL_D)) == 0)
      {
        c += strlen(MTL_D);
        mat->dissolved = strtof(c, &c);
      }
      if (strncmp(c, MTL_ILLUM, strlen(MTL_ILLUM)) == 0)
      {
        c += strlen(MTL_ILLUM);
        mat->illumModels = strtol(c, &c, 10);
      }
      if (strncmp(c, MTL_MAP_BUMP, strlen(MTL_MAP_BUMP)) == 0)
      {
        c += strlen(MTL_MAP_BUMP);
        char* name = ReadUntil(arena, c, '\n');
        mat->bumpMap = LoadImageToArena(arena, mem, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_D, strlen(MTL_MAP_D)) == 0)
      {
        c += strlen(MTL_MAP_D);
        char* name = ReadUntil(arena, c, '\n');
        mat->alphaMap = LoadImageToArena(arena, mem, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_KA, strlen(MTL_MAP_KA)) == 0)
      {
        c += strlen(MTL_MAP_KA);
        char* name = ReadUntil(arena, c, '\n');
        mat->ambientMap = LoadImageToArena(arena, mem, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_KD, strlen(MTL_MAP_KD)) == 0)
      {
        c += strlen(MTL_MAP_KD);
        c = Skip(c, ' ');
        char* name = ReadUntil(arena, c, '\n');
        mat->diffuseMap = LoadImageToArena(arena, mem, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_KS, strlen(MTL_MAP_KS)) == 0)
      {
        c += strlen(MTL_MAP_KS);
        char* name = ReadUntil(arena, c, '\n');
        mat->specularMap = LoadImageToArena(arena, mem, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_NS, strlen(MTL_MAP_NS)) == 0)
      {
        c += strlen(MTL_MAP_NS);
        char* name = ReadUntil(arena, c, '\n');
        mat->specularExponentMap = LoadImageToArena(arena, mem, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_DECAL, strlen(MTL_MAP_DECAL)) == 0)
      {
        c += strlen(MTL_MAP_DECAL);
        char* name = ReadUntil(arena, c, '\n');
        mat->decalMap = LoadImageToArena(arena, mem, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_DISP, strlen(MTL_MAP_DISP)) == 0)
      {
        c += strlen(MTL_MAP_DISP);
        char* name = ReadUntil(arena, c, '\n');
        mat->displacementMap = LoadImageToArena(arena, mem, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
    }
  }
  return result;
}

inline loaded_model DEBUGLoadObj(memory_arena* arena, game_memory* mem, char* fileName)
{
  loaded_model result = {};

  debug_read_file_result file = mem->DEBUGPlatformReadFile(fileName);
  if (file.contentSize > 0)
  {
    char* fileEnd = (char*)file.contents + file.contentSize;
    for (char* c = (char*)file.contents; c < fileEnd; ++c)
    {
      if (*c == '#') c = SkipUntil(c, '\n');
      if (c[0] == 'v' && c[1] == ' ' && (c - 1 < file.contents || *(c - 1) == '\n'))
      {
        result.posCount++;
      }
      if (c[0] == 'v' && c[1] == 't' && c[2] == ' ' && (c - 1 < file.contents || *(c - 1) == '\n'))
      {
        result.texCount++;
      }
      if (c[0] == 'v' && c[1] == 'n' && c[2] == ' ' && (c - 1 < file.contents || *(c - 1) == '\n'))
      {
        result.norCount++;
      }
      if (c[0] == 'v' && c[1] == 'n' && c[2] == ' ' && (c - 1 < file.contents || *(c - 1) == '\n'))
      {
        result.norCount++;
      }
      result.iInVert = (result.posCount > 0 ? 1 : 0) + (result.texCount > 0 ? 1 : 0) + (result.norCount > 0 ? 1 : 0);
      if (c[0] == 'f' && c[1] == ' ' && (c - 1 < file.contents || *(c - 1) == '\n'))
      {
        u32 count = 0;
        u32 slashCount = 0;
        for (; c[0] != '\n'; ++c)
        {
          if (c[0] == ' ') count++;
        }
        result.group[result.groupCount - 1].iCount += (count - 2) * 3 * result.iInVert;
      }

      if (strncmp(c, MTL_USE, strlen(MTL_USE)) == 0)
      {
        c += strlen(MTL_USE);
        c = Skip(c, ' ');
        result.group[result.groupCount].matName = ReadUntil(arena, c, '\n');
        ++result.groupCount;
      }
    }

    result.positions = result.posCount > 0 ? PUSH_ARRAY(arena, v3, result.posCount) : 0;
    result.normals = result.norCount > 0 ? PUSH_ARRAY(arena, v3, result.norCount) : 0;
    result.texCoords = result.texCount > 0 ? PUSH_ARRAY(arena, v2, result.texCount) : 0;
    for (u32 i = 0; i < result.groupCount;++i)
    {
      result.group[i].indices = PUSH_ARRAY(arena, u32, result.group[i].iCount);
    }

    u32 vIndex = 0;
    u32 vnIndex = 0;
    u32 vtIndex = 0;
    u32 index = 0;
    i32 groupIndex = -1;
    for (char* c = (char*)file.contents; c < fileEnd; ++c)
    {
      if (*c == '#') c = SkipUntil(c, '\n');
      if (c[0] == 'v' && c[1] == ' ')
      {
        u32 eIndex = 0;
        for (++c; *c != '\n';)
        {
          float value = strtof(c, &c);
          result.positions[vIndex].e[eIndex++] = value;
        }
        ++vIndex;
      }
      if (c[0] == 'v' && c[1] == 't' && c[2] == ' ')
      {
        u32 eIndex = 0;
        for (c += 2; *c != '\n';)
        {
          float value = strtof(c, &c);
          result.texCoords[vtIndex].e[eIndex++] = value;
        }
        ++vtIndex;
      }
      if (c[0] == 'v' && c[1] == 'n' && c[2] == ' ')
      {
        u32 eIndex = 0;
        for (c += 2; *c != '\n';)
        {
          float value = strtof(c, &c);
          result.normals[vnIndex].e[eIndex++] = value;
        }
        ++vnIndex;
      }
      if (c[0] == 'f' && c[1] == ' ')
      {
        //NOTE: count indices
        u32 faceCount = 0;
        for (char* t = c + 1; t[0] != '\n'; ++t)
        {
          if (t[0] == ' ') faceCount++;
        }

        //NOTE: parse indices
        u32* temp = (u32*)malloc(result.iInVert * faceCount * sizeof(u32));
        for (u32 fIndex = 0; fIndex < faceCount * result.iInVert; ++fIndex)
        {
          while (c[0] < '0' || c[0] > '9' && c[0] != '-') ++c;
          i32 value = strtol(c, &c, 10);
          temp[fIndex] = value - 1;

          ASSERT(value > 0);
        }

        //NOTE: add to model
        for (u32 i = 0; i < faceCount - 2; ++i)
        {
          for (u32 iFace = 0; iFace < result.iInVert; ++iFace)
          {
            result.group[groupIndex].indices[index++] = temp[i * result.iInVert + iFace];
          }
          for (u32 iFace = 0; iFace < result.iInVert; ++iFace)
          {
            result.group[groupIndex].indices[index++] = temp[(i + 1) * result.iInVert + iFace];
          }
          for (u32 iFace = 0; iFace < result.iInVert; ++iFace)
          {
            result.group[groupIndex].indices[index++] = temp[(faceCount - 1) * result.iInVert + iFace];
          }
        }
        free(temp);
      }
      if (strncmp(c, MTL_USE, strlen(MTL_USE)) == 0)
      {
        c += strlen(MTL_USE);
        ++groupIndex;
        index = 0;
      }
      if (strncmp(c, MTL_LIB, strlen(MTL_LIB)) == 0)
      {
        c += strlen(MTL_LIB);

        char mtlFile[64];
        sscanf_s(c, "%s", mtlFile, (unsigned int)sizeof(mtlFile));
        result.mtl = DEBUGLoadMTL(arena, mem, mtlFile);
      }
    }
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

    gameState->wall = LoadImageToArena(&gameState->arena, memory, "wall_side_left.bmp");
    gameState->knight = LoadImageToArena(&gameState->arena, memory, "knight_idle_anim_f0.bmp");

    // gameState->cube = DEBUGLoadObj(gameState, memory, "cube.obj");

    gameState->bricks = LoadImageToArena(&gameState->arena, memory, "bricks.bmp");
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
  directional_light light = {};
  light.dir = { 0.0f, -1.0f, 0.0f };
  light.diffuse = { 1.0f, 1.0f, 1.0f };
  light.ambient = 0.1f * light.diffuse;
  loaded_model cube = DEBUGLoadObj(&tranState.tranArena, memory, "cube.obj");

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

  RenderGroupOutput(memory->workQueue, renderGroup, &drawBuffer);

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

  if (!gameState->isMuted)
  {
    // PlaySound(soundBuffer, &gameState->sounds[0]);
  }
}

#endif