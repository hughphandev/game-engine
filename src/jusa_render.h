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

struct color
{
  float r, g, b, a;

  u32 ToU32();
};

struct loaded_bitmap
{
  u32* pixel;
  u32 width;
  u32 height;
};

enum render_entry_type
{
  RENDER_TYPE_render_entry_clear,
  RENDER_TYPE_render_entry_bitmap,
  RENDER_TYPE_render_entry_rectangle,
  RENDER_TYPE_render_entry_coordinate_system,
};

struct render_entry_header
{
  render_entry_type type;
};

struct render_entry_clear
{
  render_entry_header header;
  color col;
};

struct render_entry_coordinate_system
{
  render_entry_header header;

  color col;
  v3 origin;
  v3 xAxis;
  v3 yAxis;
  //TODO: use z-Axis for 3d
  v3 zAxis;

  loaded_bitmap* texture;
};

struct render_entry_rectangle
{
  render_entry_header header;
  v2 pos;
  v2 size;

  color col;
  brush_type brush;
};

struct render_entry_bitmap
{
  render_entry_header header;
  loaded_bitmap* texture;
  v2 pos;
  v2 size;

  color col;
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