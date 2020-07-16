#include "game.h"

#define PI 3.14159

void RenderGreenScreen(game_offscreen_buffer *buffer)
{
  uint8_t *row = (uint8_t *)buffer->memory;
  for (int y = 0; y < buffer->height; ++y)
  {
    uint32_t *pixel = (uint32_t *)row;
    for (int x = 0; x < buffer->width; ++x)
    {
      *pixel++ = (255 << 8);
    }
    row += buffer->pitch;
  }
}

void GameUpdateAndRender(game_offscreen_buffer *buffer, game_sound_output *soundBuffer)
{
  RenderGreenScreen(buffer);
  GameOutputSound(soundBuffer);
}

void GameOutputSound(game_sound_output *soundBuffer)
{
  static float tSine;
  int volume = 1000;
  int toneHZ = 256;
  int wavePeriod = soundBuffer->samplesPerSecond / toneHZ;
  
  int16_t *sampleOut = soundBuffer->samples;
  for (int i = 0; i < soundBuffer->sampleCount; ++i)
  {
    float sineValue = sinf(tSine);
    int16_t sampleValue = sineValue * volume;
    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;

    tSine += 2.0 * PI * 1.0 / (float)wavePeriod;
  }
}
