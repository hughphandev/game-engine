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
debug_read_file_result DEBUGPlatformReadFile(char* fileName);
void DEBUGPlatformFreeMemory(void* memory);
bool DEBUGPlatformWriteFile(char* fileName, uint32_t memorySize, void* memory);
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

struct game_input
{
  // keyboard
  char keyCode;
  bool isDown;
  bool wasDown;

  // clock
  float elapsed;
}; // TODO: Clean up

struct game_memory
{
  bool initialized;

  uint64_t permanentStorageSize;
  void* permanentStorage; // TODO: REQUIRE clear to zero on startup

  uint64_t transientStorageSize;
  void* transientStorage; // TODO: REQUIRE clear to zero on startup

bool (*DEBUGPlatformWriteFile)(char* fileName, uint32_t memorySize, void* memory);
void (*DEBUGPlatformFreeMemory)(void* memory);
debug_read_file_result (*DEBUGPlatformReadFile)(char* fileName);
};

struct Rec
{
  float x;
  float y;
  int width;
  int height;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* gameMemory, game_offscreen_buffer* buffer, game_input input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub) {}

#define GAME_OUTPUT_SOUND(name) void name(game_memory* gameMemory, game_sound_output* soundBuffer)
typedef GAME_OUTPUT_SOUND(game_output_sound);
GAME_OUTPUT_SOUND(GameOutputSoundStub) {}

extern "C" void GameUpdateAndRender(game_memory* gameMemory, game_offscreen_buffer* buffer, game_input input);
extern "C" void GameOutputSound(game_memory* gameMemory, game_sound_output* soundBuffer);

#endif
