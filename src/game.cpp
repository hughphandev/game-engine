#include "game.h"

void GameUpdateAndRender(game_offscreen_buffer *buffer)
{
  uint8_t *row = (uint8_t *)buffer->memory;
  for(int y = 0; y < buffer->height; ++y)
    {
      uint32_t *pixel = (uint32_t *)row;
      for(int x = 0; x < buffer->width; ++x)
	{
	  *pixel++ = (255 << 8);
	}
      row += buffer->pitch;
    }
}
