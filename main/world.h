#ifndef WORLD_H
#define WORLD_H

#include "types.h"
#include "math.h"

struct game_world
{
  v2 tilesInChunk;

  u32 tileCountX;
  u32 tileCountY;
  v2 tileSizeInMeter;
};

inline rec GetTileBound(game_world* world, i32 x, i32 y);
inline v2 GetChunk(v2 pos, game_world* world);
inline v2 GetTileIndex(game_world* world, float x, float y);
inline v2 GetTileIndex(game_world* world, v2 pos);

#endif