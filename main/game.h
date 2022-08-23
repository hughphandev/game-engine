#ifndef GAME_H
#define GAME_H

#include "memory.h"
#include "utils.h"
#include "world.h"
#include "render.h"
#include "math.h"
#include "thread.h"
#include "assets.h"

#if INTERNAL

enum
{
  DebugCycleCounter_GameUpdateAndRender,
  DebugCycleCounter_RenderGroupOutput,
  DebugCycleCounter_DrawTriangle,
  DebugCycleCounter_PixelFill,
  DebugCycleCounter_Count,
};

struct debug_cycle_counter
{
  u64 cycleCount;
  u32 hitCount;
};

extern struct game_memory* g_memory;
#if _MSC_VER
// #define COUNTER(ID) int ID = 0;
// #define ADD_COUNTER(ID, i) ID += i;
#define BEGIN_TIMER_BLOCK(ID) u64 startCycleCounter##ID = __rdtsc();
#define END_TIMER_BLOCK(ID) g_memory->counter[DebugCycleCounter_##ID].cycleCount += __rdtsc() - startCycleCounter##ID; ++g_memory->counter[DebugCycleCounter_##ID].hitCount;
#define END_TIMER_BLOCK_COUNTED(ID, count) g_memory->counter[DebugCycleCounter_##ID].cycleCount += __rdtsc() - startCycleCounter##ID; g_memory->counter[DebugCycleCounter_##ID].hitCount += count;
#else
// #define COUNTER(ID)
// #define ADD_COUNTER(ID, i)
#define BEGIN_TIMER_BLOCK(ID)
#define END_TIMER_BLOCK(ID) 
#define END_TIMER_BLOCK_COUNTED(ID, count) 
#endif

#endif

struct game_sound_output
{
  size_t sampleCount;
  size_t bytesPerSample;
  i16* samples;
};

struct button_state
{
  bool isDown;
  u32 halfTransitionCount;
};

struct game_input
{
  // button
  union
  {
    button_state buttons[8];
    struct
    {
      button_state up;
      button_state down;
      button_state left;
      button_state right;
      button_state escape;
      button_state space;
      button_state f1;
      button_state f3;
    };
  };

  // Mouse
  i32 mouseX, mouseY, mouseZ;
  i32 dMouseX, dMouseY;
  bool mouseButtonState[5];

  // clock
  float dt;
}; // TODO: Clean up


enum entity_type
{
  ENTITY_WALL,
  ENTITY_MONSTAR,
  ENTITY_PLAYER,
  ENTITY_SWORD,
  ENTITY_PROJECTILE
};

struct entity
{
  v3 pos;
  v3 size;
  v3 vel;

  entity_type type;

  float hp;

  bool canCollide;
  bool canUpdate;

  entity* owner;
  float lifeTime;
};

enum program_mode
{
  MODE_NORMAL,
  MODE_EDITOR,
  MODE_MENU
};



struct game_state
{
  memory_arena arena;
  camera cam;
  game_world world;

#define MAX_ENTITY_COUNT 100000
  entity entities[MAX_ENTITY_COUNT];
  size_t entityCount;

  entity* player;

  loaded_sound sounds[10];
  size_t soundCount;
  bool isMuted;
  program_mode programMode;

  u32 viewDistance;

  // Textures
  loaded_bitmap knight;
  loaded_bitmap wall;
  loaded_bitmap bricks;
  loaded_bitmap bricksNormal;

  loaded_bitmap testDiffuse;
  loaded_bitmap testNormal;

  loaded_model cube;

  float time;
};

struct transient_state
{
#define MAX_ACTIVE_ENTITY 4096
  entity** activeEntity;
  size_t activeEntityCount;
  memory_arena tranArena;

  u32 envMapWidth;
  u32 envMapHeight;
  //NOTE: 0 is bottom, 1 is middle, 2 is top
  environment_map envMap[3];

  bool isInit;
};

struct game_memory
{
  bool isInit;

  u64 permanentStorageSize;
  void* permanentStorage; // TODO: REQUIRE clear to zero on startup

  u64 transientStorageSize;
  void* transientStorage; // TODO: REQUIRE clear to zero on startup

  file_io fileIO;

  platform_add_work_entry* PlatformAddWorkEntry;
  platform_complete_all_work* PlatformCompleteAllWork;

  platform_work_queue* workQueue;

#if INTERNAL
  debug_cycle_counter counter[DebugCycleCounter_Count];
#endif
};

struct level
{
  i32 entityCount;
  entity* entities;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* memory, game_offscreen_buffer* buffer, game_input input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_OUTPUT_SOUND(name) void name(game_memory* memory, game_sound_output* soundBuffer)
typedef GAME_OUTPUT_SOUND(game_output_sound);

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender);
extern "C" GAME_OUTPUT_SOUND(GameOutputSound);

#endif
