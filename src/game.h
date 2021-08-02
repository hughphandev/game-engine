#ifndef GAME_H
#define GAME_H

#include "jusa_world.h"

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
  bool mouseButtonState[5];

  // clock
  float dt;
}; // TODO: Clean up

struct game_camera
{
  v3 offSet;
  v3 pos;
  size_t viewDistance;

  float pixelPerMeter;
};

struct loaded_bitmap
{
  u32* pixel;
  u32 width;
  u32 height;

  v2 anchor;
};

struct loaded_sound
{
  void* mem;
  size_t samplesCount;
  size_t channelsCount;

  size_t sampleIndex;
  bool isLooped;
};

struct memory_arena
{
  void* base;
  size_t used;
  size_t size;
};

struct visible_pieces
{
  loaded_bitmap pieces[10];
  size_t pieceCount;
};

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

  visible_pieces vp;
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

struct sprite_sheet
{
  loaded_bitmap sheet;
  v2 chunkSize;
};

struct game_state
{
  memory_arena arena;
  game_camera cam;
  game_world world;

#define MAX_ENTITY_COUNT 100000
  entity entities[MAX_ENTITY_COUNT];
  size_t entityCount;

  entity* player;

  loaded_bitmap textures[10];
  loaded_sound sounds[10];
  size_t soundCount;
  bool isMuted;
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
  u32 riffID;
  u32 fileSize;
  u32 waveID;
};

struct riff_chunk
{
  u32 id;
  u32 dataSize;
};
struct wav_fmt
{
  u16 audioFormat;
  u16 nChannels;
  u32 sampleRate;
  u32 byteRate;
  u16 blockAlign;
  u16 bitsPerSample;
  u16 cbSize;
  u16 validBitsPerSample;
  u32 channelMask;
  char subFormat[16];
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

struct level
{
  i32 entityCount;
  entity* entities;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* gameMemory, game_offscreen_buffer* buffer, game_input input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_OUTPUT_SOUND(name) void name(game_memory* gameMemory, game_sound_output* soundBuffer)
typedef GAME_OUTPUT_SOUND(game_output_sound);

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender);
extern "C" GAME_OUTPUT_SOUND(GameOutputSound);

#endif
