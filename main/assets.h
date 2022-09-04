#ifndef ASSETS_H
#define ASSETS_H

#include "types.h"
#include "utils.h"
#include "memory.h"
#include "math.h"
#include "intrinsics.h"

#include <string.h>
#include <stdlib.h>

#define BITMAP_PIXEL_SIZE (sizeof(u32))
struct loaded_bitmap
{
  //NOTE: ARGB
  v2 align;
  u32* pixel;
  int width;
  int height;
};

struct loaded_material
{
  char* name;
  v3 ambient;
  v3 diffuse;
  v3 specular;
  v3 emissive;
  float specularExponent;
  float dissolved;
  float opticalDensity;
  i32 illumModels;

  loaded_bitmap ambientMap;
  loaded_bitmap diffuseMap;
  loaded_bitmap specularMap;
  loaded_bitmap specularExponentMap;
  loaded_bitmap alphaMap;
  loaded_bitmap bumpMap;
  loaded_bitmap displacementMap;
  loaded_bitmap decalMap;
};

struct loaded_mtl
{
  loaded_material* materials;
  size_t matCount;
};

struct index_group
{
  char* matName;
  u32* indices;
  u32 iCount;
};

#define MAX_MATERIAL_COUNT 5
struct loaded_model
{
  v3* positions;
  u32 posCount;

  v3* normals;
  u32 norCount;

  v2* texCoords;
  u32 texCount;

  index_group group[5];
  u32 iInVert;
  u32 groupCount;

  loaded_mtl mtl;
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

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24)) 
enum
{
  WAVE_CHUNKID_FMT = RIFF_CODE('f', 'm', 't', ' '),
  WAVE_CHUNKID_DATA = RIFF_CODE('d', 'a', 't', 'a'),
  WAVE_CHUNKID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
  WAVE_CHUNKID_RIFF = RIFF_CODE('R', 'I', 'F', 'F')
};

struct loaded_sound
{
  void* mem;
  size_t samplesCount;
  size_t channelsCount;

  size_t sampleIndex;
  bool isLooped;
};

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

#endif
