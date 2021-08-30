#ifndef JUSA_MEMORY_H
#define JUSA_MEMORY_H

#include "jusa_utils.h"

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

static void InitMemoryArena(memory_arena* arena, size_t size, void* base)
{
  arena->base = (u8*)base;
  arena->size = size;
  arena->used = 0;
}


#endif