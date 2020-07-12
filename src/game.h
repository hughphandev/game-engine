#if !defined(GAME_H)

struct game_offscreen_buffer
{
  void *memory;
  int width;
  int height;
  int pitch;
};

void GameUpdateAndRender(game_offscreen_buffer * buffer);

#define GAME_H
#endif
