#ifndef GAME_H
#define GAME_H

#include "jusa_math.h"
#include "jusa_types.h"

#if INTERNAL
struct debug_read_file_result
{
  void* contents;
  u32 contentSize;
};


#define DEBUG_PLATFORM_READ_FILE(name) debug_read_file_result name(char* fileName)
typedef DEBUG_PLATFORM_READ_FILE(debug_platform_read_file);

#define DEBUG_PLATFORM_FREE_MEMORY(name)void name (void* memory) 
typedef DEBUG_PLATFORM_FREE_MEMORY(debug_platform_free_memory);

#define DEBUG_PLATFORM_WRITE_FILE(name) bool name(char* fileName, size_t memorySize, void* memory) 
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file);

DEBUG_PLATFORM_READ_FILE(DEBUGPlatformReadFile);
DEBUG_PLATFORM_FREE_MEMORY(DEBUGPlatformFreeMemory);
DEBUG_PLATFORM_WRITE_FILE(DEBUGPlatformWriteFile);
#endif

struct game_offscreen_buffer
{
  void* memory;
  int width;
  int height;
};

struct game_sound_output
{
  u32 bytesPerSample;
  u32 sampleCount;
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
    button_state buttons[7];
    struct
    {
      button_state up;
      button_state down;
      button_state left;
      button_state right;
      button_state escape;
      button_state space;
      button_state f1;
    };
  };

  // Mouse
  i32 mouseX, mouseY, mouseZ;
  bool mouseButtonState[5];

  // clock
  float dt;
}; // TODO: Clean up

union rec
{
  struct
  {
    v2 pos;
    v2 size;
  };
  struct
  {
    float x, y, width, height;
  };

  inline v2 GetMinBound();
  inline v2 GetMaxBound();
};

struct game_camera
{
  v2 offSet;
  v2 pos;

  float pixelPerMeter;
};

struct loaded_bitmap
{
  u32* pixel;
  u32 width;
  u32 height;
};

struct loaded_sound
{
  i16* samples;
  size_t sampleCount;
  size_t playIndex = 0;
  u32 channel;
  u32 sampleRate;
  bool isLooped;
};

struct memory_arena
{
  void* base;
  size_t used;
  size_t size;
};

struct game_world
{
  game_camera cam;

  u32 tileCountX;
  u32 tileCountY;
  v2 tileSizeInMeter;
};

struct entity
{
  rec hitbox;
  v2 vel;
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
  game_world world;

  entity entities[100000];
  i32 entityCount;
  i32 playerIndex;

  loaded_bitmap background;
  loaded_sound bSound;
  u32 soundIndex;
  program_mode programMode;
};

#pragma pack(push, 1)
struct bmp_header
{
  //BMP Header
  u16 signature;
  u32 fileSize;
  u16 reserved1;
  u16 reserved2;
  u32 offSet;

  //DIB Header
  u32 headerSize;
  u32 width;
  u32 height;
  u16 planes;
  u16 bitsPerPixel;
  u32 compression;
  u32 sizeOfBitMap;
  u32 printWidth;
  u32 printHeight;
  u32 colorPalette;
  u32 important;

  u32 redMask;
  u32 greenMask;
  u32 blueMask;
  u32 alphaMask;
};

struct wav_header
{
  char riffID[4];
  u32 fileSize;
  char waveID[4];
};
struct wav_fmt
{
  char fmtID[4];
  u32 fmtSize;
  u16 audioFormat;
  u16 channel;
  u32 sampleRate;
  u32 byteRate;
  u16 blockAlign;
  u16 bitPerSample;
};
struct wav_data
{
  char dataID[4];
  u32 dataSize;
};
#pragma pack(pop)


struct game_memory
{
  bool isInited;

  u64 permanentStorageSize;
  void* permanentStorage; // TODO: REQUIRE clear to zero on startup

  u64 transientStorageSize;
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
