#include "game.h"

void RenderScreen(game_offscreen_buffer* buffer, u32 color);
void DrawRectangle(game_offscreen_buffer* buffer, u32 color, rec r);

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

v2 operator*(v2 lhs, float rhs)
{
  return { lhs.x * rhs, lhs.y * rhs };
}
v2 operator/(v2 lhs, float rhs)
{
  return { lhs.x / rhs, lhs.y / rhs };
}
v2 operator+(v2 lhs, float rhs)
{
  return { lhs.x + rhs, lhs.y + rhs };
}
v2 operator-(v2 lhs, float rhs)
{
  return { lhs.x - rhs, lhs.y - rhs };
}

v2 operator*(v2 lhs, v2 rhs)
{
  return { lhs.x * rhs.x, lhs.y * rhs.y };
}
v2 operator/(v2 lhs, v2 rhs)
{
  return { lhs.x / rhs.x, lhs.y / rhs.y };
}
v2 operator+(v2 lhs, v2 rhs)
{
  return { lhs.x + rhs.x, lhs.y + rhs.y };
}
v2 operator-(v2 lhs, v2 rhs)
{
  return { lhs.x - rhs.x, lhs.y - rhs.y };
}

v2 v2::operator*=(v2 lhs)
{
  return { this->x * lhs.x, this->y * lhs.y };
}
v2 v2::operator/=(v2 lhs)
{
  return { this->x / lhs.x, this->y / lhs.y };
}
v2 v2::operator+=(v2 lhs)
{
  return { this->x + lhs.x, this->y + lhs.y };
}
v2 v2::operator-=(v2 lhs)
{
  return { this->x - lhs.x, this->y - lhs.y };
}
bool v2::operator==(v2 lhs)
{
  return (this->x == lhs.x && this->y == lhs.y);
}

v2 v2::operator*=(float lhs)
{
  return { this->x * lhs, this->y * lhs };
}
v2 v2::operator/=(float lhs)
{
  return { this->x / lhs, this->y / lhs };
}
v2 v2::operator+=(float lhs)
{
  return { this->x + lhs, this->y + lhs };
}
v2 v2::operator-=(float lhs)
{
  return { this->x - lhs, this->y - lhs };
}
bool v2::operator==(float lhs)
{
  return (this->x == lhs && this->y == lhs);
}

void Swap(float* l, float* r)
{
  float temp = *l;
  *l = *r;
  *r = temp;
}

bool RayToRec(v2 rayOrigin, v2 rayDir, rec r, v2* contactPoint, v2* contactNormal)
{
  if (rayDir == 0.0f) return false;

  v2 minPos = r.pos;
  v2 maxPos = r.pos + r.size;

  v2 tNear = (minPos - rayOrigin) / rayDir;
  v2 tFar = (maxPos - rayOrigin) / rayDir;

  if (tNear.x > tFar.x) Swap(&tNear.x, &tFar.x);
  if (tNear.y > tFar.y) Swap(&tNear.y, &tFar.y);

  if (tNear.x > tFar.y || tNear.y > tFar.x) return false;

  float tHitNear = Max(tNear.x, tNear.y);
  float tHitFar = Min(tFar.x, tFar.y);

  //Note: check if value is a floating point number!
  if (tHitNear != tHitNear) return false;

  if (tHitFar < 0.0f) return false;
  if (tHitNear > 1.0f || tHitNear < 0.0f) return false;

  contactPoint->x = rayOrigin.x + (tHitNear * rayDir.x);
  contactPoint->y = rayOrigin.y + (tHitNear * rayDir.y);

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

void DrawLine(game_offscreen_buffer* buffer, v2 from, v2 to, u32 lineColor)
{
  u32* pixel = (u32*)buffer->memory;
  if (from.x == to.x)
  {
    int startY = RoundToInt(Min(from.y, to.y));
    int endY = RoundToInt(Max(from.y, to.y));
    int x = RoundToInt(from.x);
    int screenBound = buffer->width * buffer->height;
    for (int i = startY; i < endY; ++i)
    {
      int pixelIndex = i * buffer->width + x;
      if (pixelIndex >= 0 && pixelIndex < screenBound)
      {
        pixel[i * buffer->width + x] = lineColor;
      }
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

v2 Abs(v2 v)
{
  return {Abs(v.x), Abs(v.y)};
}

void MoveAndSlide(rec* target, v2 moveVec, rec* others, int recCount)
{
  for (int i = 0; i < recCount; ++i)
  {
    rec imRec = {};
    imRec.size = others[i].size + target->size;
    imRec.pos = others[i].pos - target->size;

    v2 contactNormal = {};
    v2 contactPoint = {};

    if (RayToRec(target->pos, moveVec, imRec, &contactPoint, &contactNormal))
    {
      //Note: only true with aabb
      //TODO: solution for generic case if needed
      v2 newMoveVec = {};
      newMoveVec.x = (contactNormal.x != 0.0f) ? 0 : moveVec.x;
      newMoveVec.y = (contactNormal.y != 0.0f) ? 0 : moveVec.y;
      target->pos = contactPoint + newMoveVec;
      return;
    }
  }
  target->pos = target->pos + moveVec;
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

  v2 velocity = { };
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

  rec r[] = { { 200, 100, 100, 100 }, {400, 200, 100, 200} };
  u32 recColor = 0x00FFFF;

  MoveAndSlide(player, velocity * input.timeStep, r, 2);

  RenderScreen(buffer, 0);
  v2 mousePos = { (float)input.mouseX, (float)input.mouseY };
  DrawRectangle(buffer, recColor, r[0]);
  DrawRectangle(buffer, recColor, r[1]);
  DrawRectangle(buffer, 0xFFFFFF, *player);
}

extern "C" GAME_OUTPUT_SOUND(GameOutputSound)
{
  game_state* gameState = (game_state*)gameMemory->permanentStorage;
  gameState->volume = 10000;
  gameState->toneHZ = 256;

  int wavePeriod = soundBuffer->samplesPerSecond / gameState->toneHZ;

  i16* sampleOut = soundBuffer->samples;
  for (int i = 0; i < soundBuffer->sampleCount; ++i)
  {
    float sineValue = sinf(gameState->tSine);
    i16 sampleValue = (i16)(sineValue * gameState->volume);
    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;

    gameState->tSine += (float)(2.0 * PI * 1.0 / (float)wavePeriod);
    if (gameState->tSine > 2.0 * PI)
    {
      gameState->tSine -= 2.0f * PI;
    }
  }
}

void RenderScreen(game_offscreen_buffer* buffer, u32 color)
{
  u8* row = (u8*)buffer->memory;
  for (int y = 0; y < buffer->height; ++y)
  {
    u32* pixel = (u32*)row;
    for (int x = 0; x < buffer->width; ++x)
    {
      *pixel++ = color;
    }
    row += buffer->pitch;
  }
}


void DrawRectangle(game_offscreen_buffer* buffer, u32 color, rec r)
{
  int xMin = RoundToInt(r.pos.x);
  int yMin = RoundToInt(r.pos.y);
  int xMax = RoundToInt(r.pos.x + r.size.x);
  int yMax = RoundToInt(r.pos.y + r.size.y);
  u32* mem = (u32*)buffer->memory;
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
