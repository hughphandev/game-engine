#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <math.h>

#if INTERNAL
struct debug_read_file_result
{
  void* contents;
  uint32_t contentSize;
};

#define DEBUG_PLATFORM_READ_FILE(name) debug_read_file_result name(char* fileName)
typedef DEBUG_PLATFORM_READ_FILE(debug_platform_read_file);

#define DEBUG_PLATFORM_FREE_MEMORY(name)void name (void* memory) 
typedef DEBUG_PLATFORM_FREE_MEMORY(debug_platform_free_memory);

#define DEBUG_PLATFORM_WRITE_FILE(name) bool name(char* fileName, uint32_t memorySize, void* memory) 
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file);

DEBUG_PLATFORM_READ_FILE(DEBUGPlatformReadFile);
DEBUG_PLATFORM_FREE_MEMORY(DEBUGPlatformFreeMemory);
DEBUG_PLATFORM_WRITE_FILE(DEBUGPlatformWriteFile);
#endif

#if SLOW
#define ASSERT(Expression) if(!(Expression)) {*(int *)0 = 0;}
inline uint32_t SafeTruncateUInt64(uint64_t value);
#else
#define ASSERT(Expression) 
#endif

#define KILOBYTES(value) ((value) * 1024LL) 
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL) 
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL) 
#define TERABYTES(value) (GIGABYTES(value) * 1024LL) 

struct game_offscreen_buffer
{
  void* memory;
  int width;
  int height;
  int pitch;
};

struct game_sound_output
{
  int samplesPerSecond;
  int sampleCount;
  int16_t* samples;
};

struct button_state
{
  bool isDown;
  uint32_t halfTransitionCount;
};

struct game_input
{
  // button
  union
  {
    button_state buttons[4];
    struct
    {
      button_state up;
      button_state down;
      button_state left;
      button_state right;
    };
  };

  // Mouse
  int mouseX, mouseY, mouseZ;
  bool mouseButtonState[5];

  // clock
  float timeStep;
}; // TODO: Clean up

struct vector2
{
  float x, y;
};

struct rec
{
  vector2 pos;
  vector2 size;
};

struct game_state
{
  float tSine;
  int volume;
  int toneHZ;

  rec player;
};

struct game_memory
{
  bool initialized;

  uint64_t permanentStorageSize;
  void* permanentStorage; // TODO: REQUIRE clear to zero on startup

  uint64_t transientStorageSize;
  void* transientStorage; // TODO: REQUIRE clear to zero on startup

  debug_platform_free_memory* DEBUGPlatformFreeMemory;
  debug_platform_read_file* DEBUGPlatformReadFile;
  debug_platform_write_file* DEBUGPlatformWriteFile;
};


#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* gameMemory, game_offscreen_buffer* buffer, game_input input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_OUTPUT_SOUND(name) void name(game_memory* gameMemory, game_sound_output* soundBuffer)
typedef GAME_OUTPUT_SOUND(game_output_sound);

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender);
extern "C" GAME_OUTPUT_SOUND(GameOutputSound);

#endif
