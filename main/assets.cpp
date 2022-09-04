#ifndef ASSETS_CPP
#define ASSETS_CPP

#include "assets.h"

#define STB_IMAGE_IMPLEMENTATION
#include "3rd/stb/stb_image.h"

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

inline loaded_bitmap DEBUGLoadBMP(memory_arena* arena, platform_api fileIO, char* fileName)
{
  loaded_bitmap result = {};

  debug_read_file_result file = fileIO.ReadFile(fileName);
  if (file.contentSize > 0)
  {
    bmp_header* header = (bmp_header*)file.contents;
    u32* filePixel = (u32*)((u8*)file.contents + header->offSet);

    result.pixel = PUSH_ARRAY(arena, u32, header->width * header->height);
    result.width = header->width;
    result.height = header->height;

    bit_scan_result redScan = FindLowestSetBit(header->redMask);
    bit_scan_result greenScan = FindLowestSetBit(header->greenMask);
    bit_scan_result blueScan = FindLowestSetBit(header->blueMask);
    bit_scan_result alphaScan = FindLowestSetBit(header->alphaMask);

    ASSERT(redScan.found);
    ASSERT(blueScan.found);
    ASSERT(greenScan.found);

    u32 alphaShilf = (u32)alphaScan.index;
    u32 redShilf = (u32)redScan.index;
    u32 greenShilf = (u32)greenScan.index;
    u32 blueShilf = (i32)blueScan.index;

    u32 index = 0;
    for (u32 y = 0; y < header->height; ++y)
    {
      for (u32 x = 0; x < header->width; ++x)
      {
        u32 red = (filePixel[index] & header->redMask) >> redShilf;
        u32 green = (filePixel[index] & header->greenMask) >> greenShilf;
        u32 blue = (filePixel[index] & header->blueMask) >> blueShilf;

        u32 alpha = (alphaScan.found) ? (filePixel[index] & header->alphaMask) >> alphaShilf : 0xFF;
        float destA = (float)alpha / 255.0f;

        //NODE: Premultiplied Alpha
        red = (u32)((float)red * destA);
        green = (u32)((float)green * destA);
        blue = (u32)((float)blue * destA);

        result.pixel[index] = (red << 16) | (green << 8) | (blue << 0) | (alpha << 24);
        ++index;
      }
    }
    fileIO.FreeFile(file.contents);
  }
  return result;
}

inline loaded_bitmap LoadImageToArena(memory_arena* arena, platform_api fileIO, char* fileName)
{
  loaded_bitmap result = {};
  int channel;
  stbi_set_flip_vertically_on_load(1);
  u32* pixel = (u32*)stbi_load(fileName, &result.width, &result.height, &channel, 4);
  result.pixel = PUSH_ARRAY(arena, u32, result.width * result.height);
  size_t index = 0;
  for (size_t y = 0; y < result.height; ++y)
  {
    for (size_t x = 0; x < result.width; ++x)
    {
      u32 red = (pixel[index] >> 0) & 0xFF;
      u32 green = (pixel[index] >> 8) & 0xFF;
      u32 blue = (pixel[index] >> 16) & 0xFF;
      u32 alpha = (pixel[index] >> 24) & 0xFF;

      float fAlpha = (float)alpha / 255.0f;

      //NODE: Premultiplied Alpha
      red = (u32)((float)red * fAlpha);
      green = (u32)((float)green * fAlpha);
      blue = (u32)((float)blue * fAlpha);

      result.pixel[index] = (red << 16) | (green << 8) | (blue << 0) | (alpha << 24);
      ++index;
    }
  }
  stbi_image_free(pixel);
  return result;
}

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

inline loaded_sound DEBUGLoadWAV(memory_arena* arena, platform_api fileIO, char* fileName)
{
  loaded_sound result = {};
  debug_read_file_result file = fileIO.ReadFile(fileName);
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

    result.samplesCount = dataSize / bytesPerSample;
    result.channelsCount = nChannels;
    result.mem = PUSH_SIZE(arena, dataSize);
    memcpy(result.mem, data, dataSize);
  }
  fileIO.FreeFile(file.contents);
  return result;
}



inline loaded_mtl DEBUGLoadMTL(memory_arena* arena, platform_api fileIO, char* fileName)
{
  loaded_mtl result = {};
  //TODO: load mtl file
  debug_read_file_result file = fileIO.ReadFile(fileName);
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
        mat->bumpMap = LoadImageToArena(arena, fileIO, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_D, strlen(MTL_MAP_D)) == 0)
      {
        c += strlen(MTL_MAP_D);
        char* name = ReadUntil(arena, c, '\n');
        mat->alphaMap = LoadImageToArena(arena, fileIO, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_KA, strlen(MTL_MAP_KA)) == 0)
      {
        c += strlen(MTL_MAP_KA);
        char* name = ReadUntil(arena, c, '\n');
        mat->ambientMap = LoadImageToArena(arena, fileIO, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_KD, strlen(MTL_MAP_KD)) == 0)
      {
        c += strlen(MTL_MAP_KD);
        c = Skip(c, ' ');
        char* name = ReadUntil(arena, c, '\n');
        mat->diffuseMap = LoadImageToArena(arena, fileIO, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_KS, strlen(MTL_MAP_KS)) == 0)
      {
        c += strlen(MTL_MAP_KS);
        char* name = ReadUntil(arena, c, '\n');
        mat->specularMap = LoadImageToArena(arena, fileIO, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_NS, strlen(MTL_MAP_NS)) == 0)
      {
        c += strlen(MTL_MAP_NS);
        char* name = ReadUntil(arena, c, '\n');
        mat->specularExponentMap = LoadImageToArena(arena, fileIO, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_DECAL, strlen(MTL_MAP_DECAL)) == 0)
      {
        c += strlen(MTL_MAP_DECAL);
        char* name = ReadUntil(arena, c, '\n');
        mat->decalMap = LoadImageToArena(arena, fileIO, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
      if (strncmp(c, MTL_MAP_DISP, strlen(MTL_MAP_DISP)) == 0)
      {
        c += strlen(MTL_MAP_DISP);
        char* name = ReadUntil(arena, c, '\n');
        mat->displacementMap = LoadImageToArena(arena, fileIO, name);
        FREE_ARRAY(arena, name, char, strlen(name) + 1);
      }
    }
  }
  fileIO.FreeFile(file.contents);
  return result;
}

inline loaded_model DEBUGLoadObj(memory_arena* arena, platform_api fileIO, char* fileName)
{
  loaded_model result = {};

  debug_read_file_result file = fileIO.ReadFile(fileName);
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
        result.mtl = DEBUGLoadMTL(arena, fileIO, mtlFile);
      }
    }
  }
  fileIO.FreeFile(file.contents);
  return result;
}

#endif