#if !defined(GAME_H)

struct game_offscreen_buffer
{
  void *memory;
  int width;
  int height;
  int pitch;
};
struct game_sound_output
{
  int samplesPerSecond;
  int sampleCount;
  int16_t *samples;
};

void GameUpdateAndRender(game_offscreen_buffer * buffer);
void GameOutputSound(game_sound_output *soundBuffer);

#define GAME_H
#endif
