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

enum brush_type
{
  BRUSH_FILL,
  BRUSH_WIREFRAME
};


struct loaded_bitmap
{
#define BITMAP_PIXEL_SIZE (sizeof(u32))
  u32* pixel;
  int width;
  int height;
};

struct environment_map
{
  loaded_bitmap LOD[4];
};

enum render_entry_type
{
  RENDER_TYPE_render_entry_clear,
  RENDER_TYPE_render_entry_bitmap,
  RENDER_TYPE_render_entry_rectangle,
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

struct render_entry_coordinate_system
{
  v4 col;
  v3 origin;
  v3 xAxis;
  v3 yAxis;
  //TODO: use z-Axis for 3d
  v3 zAxis;

  loaded_bitmap* texture;
  loaded_bitmap* normalMap;

  environment_map* top;
  environment_map* middle;
  environment_map* bottom;
};

struct render_entry_rectangle
{
  v2 pos;
  v2 size;

  v4 col;
  brush_type brush;
};

struct render_entry_bitmap
{
  loaded_bitmap* texture;
  v2 pos;
  v2 size;

  v4 color;
};

struct camera
{
  v3 offSet;
  v3 pos;

  float pixelPerMeter;
};

struct render_group
{
  camera* cam;

  memory_arena pushBuffer;
};


#endif