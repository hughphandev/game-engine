#ifndef JUSA_RENDER_H
#define JUSA_RENDER_H

#include "jusa_types.h"
#include "jusa_utils.h"
#include "jusa_math.h"
#include "jusa_memory.h"

struct game_offscreen_buffer
{
  void* memory;
  int width;
  int height;
};


#define BITMAP_PIXEL_SIZE (sizeof(u32))
struct loaded_bitmap
{
  v2 align;
  u32* pixel;
  int width;
  int height;
};

struct environment_map
{
  loaded_bitmap LOD[4];
  float pZ;
};

struct vertex
{
  //NOTE: assume pos in NDC
  v3 pos;
  v2 uv;
};
DEFINE_SWAP(vertex)

union triangle
{
  struct
  {
    vertex p0, p1, p2;
  };
  vertex p[3];
};

union flat_quad
{
  //NOTE: assume p1, p2 has the same y in screen coordinate
  vertex p[4];
  struct
  {
    vertex p0, p1, p2, p3;
  };
};

enum render_entry_type
{
  RENDER_TYPE_render_entry_clear,
  RENDER_TYPE_render_entry_bitmap,
  RENDER_TYPE_render_entry_rectangle,
  RENDER_TYPE_render_entry_rectangle_outline,
  RENDER_TYPE_render_entry_coordinate_system,
  RENDER_TYPE_render_entry_saturation,
};

struct render_entry_header
{
  render_entry_type type;
};

struct render_entry_clear
{
  v4 color;
};

struct render_entry_saturation
{
  float level;
};

//NOTE: for testing purpose only
struct render_entry_coordinate_system
{
  v3 point[4];

  v4 col;
  loaded_bitmap* bitmap;
  loaded_bitmap* normalMap;

  environment_map* top;
  environment_map* middle;
  environment_map* bottom;
};

struct render_entry_rectangle
{
  v2 minP;
  v2 maxP;
  v4 color;
};

struct render_entry_rectangle_outline
{
  v2 minP;
  v2 maxP;
  v4 color;
  u32 thickness;
};

struct render_entry_bitmap
{
  loaded_bitmap* bitmap;
  v2 minP;
  v2 maxP;
  v4 color;
};

struct camera
{
  v3 offSet;
  v3 pos;
  v2 rot;

  float meterToPixel;

  float zNear, zFar;
  float rFov;
  v2 size;
  v3 dir;
};

struct render_group
{
  camera* cam;
  memory_arena pushBuffer;
};


#endif