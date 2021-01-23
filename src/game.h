#ifndef GAME_H
#define GAME_H


#if INTERNAL
struct debug_read_file_result
{
  void* contents;
  uint32_t contentSize;
};
static debug_read_file_result DEBUGPlatformReadFile(char* fileName);
static void DEBUGPlatformFreeMemory(void* memory);

static bool DEBUGPlatformWriteFile(char* fileName, uint32_t memorySize, void* memory);
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
};

void GameUpdateAndRender(game_memory* gameMemory, game_offscreen_buffer* buffer,
                         game_input input);
void GameOutputSound(game_memory* gameMemory, game_sound_output* soundBuffer);

#endif
