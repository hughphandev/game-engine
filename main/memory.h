#ifndef MEMORY_H
#define MEMORY_H

#include "utils.h"

struct memory_arena
{
  void* base;
  size_t used;
  size_t size;
};

#define PUSH_ARRAY(arena, type, count) (type*)PushSize_(arena, sizeof(type) * (count))
#define PUSH_TYPE(arena, type) (type*)PushSize_(arena, sizeof(type))
#define PUSH_SIZE(arena, size) PushSize_(arena, size)
static void* PushSize_(memory_arena* arena, size_t bytes)
{
  ASSERT((arena->size - arena->used) >= bytes);

  void* result = (u8*)arena->base + arena->used;
  arena->used += bytes;
  return result;
}

#define FREE_ARRAY(arena, location, type, count) FreeSize_(arena, location, sizeof(type) * (count))
#define FREE_TYPE(arena, location, type) FreeSize_(arena, location, sizeof(type))
#define FREE_SIZE(arena, location, size) FreeSize_(arena, location, size)
static void FreeSize_(memory_arena* arena, void* location, size_t sizeInByte)
{
  ASSERT((size_t)((u8*)location - (u8*)arena->base) <= arena->used);

  size_t copySize = ((u8*)arena->base + arena->used) - ((u8*)location + sizeInByte);
  memcpy(location, (u8*)location + sizeInByte + 1, copySize);
  arena->used -= sizeInByte;
}

static void InitMemoryArena(memory_arena* arena, size_t size, void* base)
{
  arena->base = (u8*)base;
  arena->size = size;
  arena->used = 0;
}


#endif