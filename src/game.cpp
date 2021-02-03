#include "game.h"
#include <stdio.h>

void RenderScreen(game_offscreen_buffer* buffer, uint32_t color);
void RenderRectangle(game_offscreen_buffer* buffer, uint32_t color, int x, int y, int width, int height);

#define PI 3.14159f

extern "C" void GameUpdateAndRender(game_memory * gameMemory, game_offscreen_buffer * buffer,
                                    game_input input)
{
  if (!gameMemory->initialized)
  {
    char* fileName = __FILE__;
    debug_read_file_result file = gameMemory->DEBUGPlatformReadFile(fileName);
    if (file.contents)
    {
      gameMemory->DEBUGPlatformWriteFile("w:/game_engine/game_engine/build/test.out", file.contentSize, file.contents);
      gameMemory->DEBUGPlatformFreeMemory(file.contents);
    }

    gameMemory->initialized = true;
  }

  game_state* gameState = (game_state*)gameMemory->permanentStorage;
  Rec* player = &gameState->player;
  player->width = 10;
  player->height = 10;
  if (input.keyCode == 'D' && input.isDown)
  {
    player->x += 200.0f * input.elapsed;
  }
  if (input.keyCode == 'W' && input.isDown)
  {
    player->y -= 200.0f * input.elapsed;
  }
  if (input.keyCode == 'S' && input.isDown)
  {
    player->y += 200.0f * input.elapsed;
  }
  if (input.keyCode == 'A' && input.isDown)
  {
    player->x -= 200.0f * input.elapsed;
  }
  RenderScreen(buffer, 0);
  RenderRectangle(buffer, 0xFFFFFFF, (int)player->x, (int)player->y, (int)player->width, (int)player->height);
}

extern "C" void GameOutputSound(game_memory * gameMemory, game_sound_output * soundBuffer)
{
  game_state* gameState = (game_state*)gameMemory->permanentStorage;
  gameState->volume = 10000;
  gameState->toneHZ = 256;

  int wavePeriod = soundBuffer->samplesPerSecond / gameState->toneHZ;

  int16_t* sampleOut = soundBuffer->samples;
  for (int i = 0; i < soundBuffer->sampleCount; ++i)
  {
    float sineValue = sinf(gameState->tSine);
    int16_t sampleValue = (int16_t)(sineValue * gameState->volume);
    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;

    gameState->tSine += (float)(2.0 * PI * 1.0 / (float)wavePeriod);
    if (gameState->tSine > 2.0 * PI)
    {
      gameState->tSine -= 2.0f * PI;
    }
  }
}

void RenderScreen(game_offscreen_buffer* buffer, uint32_t color)
{
  uint8_t* row = (uint8_t*)buffer->memory;
  for (int y = 0; y < buffer->height; ++y)
  {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = 0; x < buffer->width; ++x)
    {
      *pixel++ = color;
    }
    row += buffer->pitch;
  }
}

void RenderRectangle(game_offscreen_buffer* buffer, uint32_t color, int x, int y, int width, int height)
{
  uint32_t* mem = (uint32_t*)buffer->memory;
  for (int w = 0; w < width; w++)
  {
    for (int h = 0; h < height; h++)
    {
      int offsetX = w + x;
      int offsetY = h + y;
      if (offsetX >= 0 && offsetX < buffer->width &&
          offsetY >= 0 && offsetY < buffer->height)
      {
        mem[offsetY * buffer->width + offsetX] = color;
      }
    }
  }
}
