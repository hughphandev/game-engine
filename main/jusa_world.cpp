#include "jusa_math.cpp"
#include "jusa_world.h"

inline rec GetTileBound(game_world* world, i32 x, i32 y)
{
  //NOTE: x, y start at top left corner!
  //TODO: Test
  rec result = {};

  result.size = world->tileSizeInMeter;

  v2 halfTileMapSizeInMeter = V2((float)world->tileCountX, (float)world->tileCountY) / 2.0f;

  v2 tilePos = { (float)x, (float)(world->tileCountY - y - 1) };

  result.pos = (tilePos - halfTileMapSizeInMeter) * world->tileSizeInMeter;

  return result;
}

inline v2 GetChunk(v2 pos, game_world* world)
{
  return Round(pos / (world->tilesInChunk * world->tileSizeInMeter));
}

inline v2 GetTileIndex(game_world* world, float x, float y)
{
  //TODO: Test
  v2 halfTileCount = (0.5f * V2((float)world->tileCountX, (float)world->tileCountY));
  return (V2(x, y) / world->tileSizeInMeter) + halfTileCount;
}

inline v2 GetTileIndex(game_world* world, v2 pos)
{
  return GetTileIndex(world, pos.x, pos.y);
}