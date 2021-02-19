#include "game.h"

void RenderScreen(game_offscreen_buffer* buffer, uint32_t color);
void DrawRectangle(game_offscreen_buffer* buffer, uint32_t color, rec r);

#define PI 3.14159f

bool IsBoxOverlapping(rec a, rec b)
{
  bool result = false;

  result = (a.pos.x < b.pos.x + b.size.x) && (a.pos.x + a.size.x > b.pos.x) && (a.pos.y + a.size.y > b.pos.y) && (a.pos.y < b.pos.y + b.size.y);

  return result;
}

float Abs(float value)
{
  return (value > 0) ? value : -value;
}

float Min(float a, float b)
{
  return a < b ? a : b;
}

static int RoundToInt(float value)
{
  return (int)(value + 0.5f);
}

float Max(float a, float b)
{
  return a > b ? a : b;
}

vector2 operator+(vector2 l, vector2 r)
{
  vector2 result = {};
  result.x = l.x + r.x;
  result.y = l.y + r.y;
  return result;
}

vector2 operator-(vector2 l, vector2 r)
{
  vector2 result = {};
  result.x = l.x - r.x;
  result.y = l.y - r.y;
  return result;
}

vector2 operator/(vector2 l, vector2 r)
{
  vector2 result = {};
  result.x = l.x / r.x;
  result.y = l.y / r.y;
  return result;
}

vector2 operator/(vector2 l, float r)
{
  vector2 result = {};
  result.x = l.x / r;
  result.y = l.y / r;
  return result;
}

vector2 operator*(vector2 l, float r)
{
  vector2 result = {};
  result.x = l.x * r;
  result.y = l.y * r;
  return result;
}

vector2 operator*(vector2 l, vector2 r)
{
  vector2 result = {};
  result.x = l.x * r.x;
  result.y = l.y * r.y;
  return result;
}

void Swap(float* l, float* r)
{
  float temp = *l;
  *l = *r;
  *r = temp;
}

bool RayToRec(vector2 rayOrigin, vector2 rayDir, rec r, vector2* contactPoint, vector2* contactNormal)
{
  vector2 minPos = r.pos - (r.size / 2.0f);
  vector2 maxPos = r.pos + (r.size / 2.0f);

  vector2 tNear = (minPos - rayOrigin) / rayDir;
  vector2 tFar = (maxPos - rayOrigin) / rayDir;

  if (tNear.x > tFar.x) Swap(&tNear.x, &tFar.x);
  if (tNear.y > tFar.y) Swap(&tNear.y, &tFar.y);

  if (tNear.x > tFar.y || tNear.y > tFar.x) return false;

  float tHitNear = Max(tNear.x, tNear.y);
  float tHitFar = Min(tFar.x, tFar.y);

  if (tHitFar < 0.0f) return false;
  if (tHitNear > 1.0f) return false;

  //TODO: right and bottom edge collision
  contactPoint->x = rayOrigin.x + tHitNear * rayDir.x;
  contactPoint->y = rayOrigin.y + tHitNear * rayDir.y;

  if (tNear.x > tNear.y)
  {
    if (rayDir.x > 0.0)
    {
      *contactNormal = { -1.0f, 0.0f };
    }
    else
    {
      *contactNormal = { 1.0f, 0.0f };
    }
  }
  else
  {
    if (rayDir.y < 0.0f)
    {
      *contactNormal = { 0.0f, 1.0f };
    }
    else
    {
      *contactNormal = { 0.0f, -1.0f };
    }
  }

  return true;
}

void DrawLine(game_offscreen_buffer* buffer, vector2 from, vector2 to, uint32_t lineColor)
{
  //TODO: Handle out of screen bound
  uint32_t* pixel = (uint32_t*)buffer->memory;
  if (from.x == to.x)
  {
    int startY = RoundToInt(Min(from.y, to.y));
    int endY = RoundToInt(Max(from.y, to.y));
    int x = RoundToInt(from.x);
    for (int i = startY; i < endY; ++i)
    {
      pixel[i * buffer->width + x] = lineColor;
    }
  }
  else
  {
    float a = (to.y - from.y) / (to.x - from.x);
    float b = from.y - (a * from.x);
    int startX = RoundToInt(Min(from.x, to.x));
    int endX = RoundToInt(Max(from.x, to.x));
    for (int i = startX; i < endX; ++i)
    {
      int y = RoundToInt(a * (float)i + b);
      if (i >= 0 && i < buffer->width && y >= 0 && y < buffer->height)
      {
        pixel[y * buffer->width + i] = lineColor;
      }
    }
  }

}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  if (!gameMemory->initialized)
  {
    gameMemory->initialized = true;
  }

  game_state* gameState = (game_state*)gameMemory->permanentStorage;

  rec* player = &gameState->player;
  player->size.x = 10;
  player->size.y = 10;

  vector2 velocity = { };
  if (input.right.isDown)
  {
    velocity.x = 200.0f;
  }
  if (input.up.isDown)
  {
    velocity.y = -200.0f;
  }
  if (input.down.isDown)
  {
    velocity.y = 200.0f;
  }
  if (input.left.isDown)
  {
    velocity.x = -200.0f;
  }

  rec r = { 200, 100, 100, 100 };
  uint32_t recColor = 0x00FFFF;

  RenderScreen(buffer, 0);
  vector2 contactNormal = {};
  vector2 contactPoint = {};
  vector2 mousePos = { (float)input.mouseX, (float)input.mouseY };
  if (RayToRec(player->pos, velocity * input.timeStep, r, &contactPoint, &contactNormal))
  {
    recColor = 0xFF0000;
    player->pos = contactPoint + (contactNormal * (player->size / 2.0f));
  }
  else
  {
    player->pos = player->pos + velocity * input.timeStep;
  }

  DrawRectangle(buffer, recColor, r);
  DrawRectangle(buffer, 0xFFFFFF, *player);
}

extern "C" GAME_OUTPUT_SOUND(GameOutputSound)
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


void DrawRectangle(game_offscreen_buffer* buffer, uint32_t color, rec r)
{
  int xMin = RoundToInt(r.pos.x - (r.size.x / 2.0f));
  int yMin = RoundToInt(r.pos.y - (r.size.y / 2.0f));
  int xMax = RoundToInt(r.pos.x + (r.size.x / 2.0f));
  int yMax = RoundToInt(r.pos.y + (r.size.y / 2.0f));
  uint32_t* mem = (uint32_t*)buffer->memory;
  for (int x = xMin; x < xMax; x++)
  {
    for (int y = yMin; y < yMax; y++)
    {
      if (x >= 0 && x < buffer->width &&
          y >= 0 && y < buffer->height)
      {
        mem[y * buffer->width + x] = color;
      }
    }
  }
}
