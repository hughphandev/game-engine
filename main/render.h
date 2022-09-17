#ifndef RENDER_H
#define RENDER_H

#include "types.h"
#include "utils.h"
#include "math.h"
#include "memory.h"
#include "thread.h"
#include "assets.h"

struct game_offscreen_buffer
{
  int width;
  int height;
  void* memory;
};

struct environment_map
{
  loaded_bitmap LOD[4];
  float pZ;
};

struct vertex
{
  v3 pos;
  v2 uv;
};

inline vertex operator-(vertex a, vertex b)
{
  vertex result;
  result.pos = a.pos - b.pos;
  result.uv = a.uv - b.uv;
  return result;
}
DEFINE_SWAP(vertex)

union triangle
{
  struct
  {
    vertex p0, p1, p2;
  };
  vertex p[3];
};

struct vertex_line
{
  vertex origin;
  vertex dir;
};

struct line
{
  v3 origin;
  v3 dir;
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


struct directional_light
{
  v3 dir;
  v3 diffuse;
  v3 ambient;
  float intensity;
};

struct point_light
{
  v3 pos;
  v3 diffuse;
  v3 attenuation;
  float intensity;
};

struct light_config
{
  directional_light* directionalLights;
  u32 directionalLightsCount;
  point_light* pointLights;
  u32 pointLightsCount;
};

enum render_entry_type
{
  RENDER_TYPE_render_entry_clear,
  RENDER_TYPE_render_entry_bitmap,
  RENDER_TYPE_render_entry_rectangle,
  RENDER_TYPE_render_entry_rectangle_outline,
  RENDER_TYPE_render_entry_saturation,
  RENDER_TYPE_render_entry_model,
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

struct render_entry_model
{
  loaded_model* model;
  light_config light;

  v4 col;
  loaded_bitmap* bitmap;
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

  float* zBuffer;

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