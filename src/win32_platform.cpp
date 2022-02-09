
#include "game.h"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>


#include <math.h>
#include <stdint.h>

#include "win32_platform.h"
#include "jusa_utils.h"
#include "jusa_thread.h"

// TODO: Global for now
static bool g_running;
static bool g_pause;
static bool g_audioSyncDisplay;
static win32_offscreen_buffer g_backBuffer;
static LPDIRECTSOUNDBUFFER g_secondaryBuffer;
static HCURSOR g_Cursor;
static bool g_IsCursorDisplay;
static WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

inline FILETIME Win32GetLastFileTime(char* fileName)
{
  FILETIME result = {};
  WIN32_FILE_ATTRIBUTE_DATA findData = {};
  if (GetFileAttributesExA(fileName, GetFileExInfoStandard, &findData))
  {
    result = findData.ftLastWriteTime;
  }

  return result;
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub) {}
GAME_OUTPUT_SOUND(GameOutputSoundStub) {}
static win32_game_code Win32LoadGameCode(char* gameDllFullPath, char* tempGameDllFullPath)
{
  win32_game_code gameCode = {};

  // Note: Wait for compiler to unlock the dll
  while (!CopyFile(gameDllFullPath, tempGameDllFullPath, FALSE));

  gameCode.lastWriteTime = Win32GetLastFileTime(gameDllFullPath);

  gameCode.gameDll = LoadLibraryA(tempGameDllFullPath);
  if (gameCode.gameDll)
  {
    gameCode.UpdateAndRender =
      (game_update_and_render*)GetProcAddress(gameCode.gameDll, "GameUpdateAndRender");
    gameCode.OutputSound =
      (game_output_sound*)GetProcAddress(gameCode.gameDll, "GameOutputSound");

    gameCode.isValid = (gameCode.UpdateAndRender && gameCode.OutputSound);
  }

  if (!gameCode.isValid)
  {
    gameCode.UpdateAndRender = GameUpdateAndRenderStub;
    gameCode.OutputSound = GameOutputSoundStub;
  }
  return gameCode;
}

static void Win32UnloadGameCode(win32_game_code* gameCode)
{
  if (gameCode->gameDll)
  {
    FreeLibrary(gameCode->gameDll);
  }

  gameCode->isValid = false;
  gameCode->UpdateAndRender = GameUpdateAndRenderStub;
  gameCode->OutputSound = GameOutputSoundStub;
}

// static void Win32LoadXInput()
// {
//   HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
//   if (!xInputLibrary)
//   {
//     xInputLibrary = LoadLibraryA("xinput1_3.dll");
//   }
//   if (xInputLibrary)
//   {
//     XInputGetState =
//       (x_input_get_state*)GetProcAddress(xInputLibrary, "XInputGetState");
//     XInputSetState =
//       (x_input_set_state*)GetProcAddress(xInputLibrary, "XInputSetState");
//   }
// }

void DEBUGPlatformFreeMemory(void* memory)
{
  VirtualFree(memory, 0, MEM_RELEASE);
}

debug_read_file_result DEBUGPlatformReadFile(char* fileName)
{
  debug_read_file_result result = {};
  HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (fileHandle != INVALID_HANDLE_VALUE)
  {
    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(fileHandle, &fileSize))
    {
      uint32_t fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);
      result.contents = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
      if (result.contents)
      {
        DWORD bytesRead;
        if (ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) && fileSize32 == bytesRead)
        {
          result.contentSize = bytesRead;
        }
        else
        {
          DEBUGPlatformFreeMemory(result.contents);
          result = {};
        }
      }
      else
      {
        // TODO: Logging
      }
    }
    else
    {
      // TODO: Logging
    }
    CloseHandle(fileHandle);
  }
  else
  {
    // TODO: Logging
  }
  return result;
}

bool DEBUGPlatformWriteFile(char* fileName, size_t memorySize, void* memory)
{
  bool result = false;
  HANDLE fileHandle = CreateFileA(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (fileHandle != INVALID_HANDLE_VALUE)
  {
    DWORD bytesWrite;
    if (WriteFile(fileHandle, memory, (DWORD)memorySize, &bytesWrite, 0))
    {
      result = (bytesWrite == memorySize);
      //TODO: File write sucessfully
    }
    else
    {
      //TODO: Logging
    }
    CloseHandle(fileHandle);
  }
  else
  {
    //TODO: Logging
  }
  return result;
}

static win32_window_dimension Win32GetWindowDimension(HWND window)
{
  RECT clientRect;
  GetClientRect(window, &clientRect);
  win32_window_dimension wd;
  wd.width = clientRect.right - clientRect.left;
  wd.height = clientRect.bottom - clientRect.top;
  return wd;
}

static void Win32ResizeDIBSection(win32_offscreen_buffer* buffer, int width, int height)
{
  if (buffer->memory)
  {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  buffer->width = width;
  buffer->height = height;
  int bytePerPixel = 4;

  buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
  buffer->info.bmiHeader.biWidth = buffer->width;
  buffer->info.bmiHeader.biHeight = buffer->height;
  buffer->info.bmiHeader.biPlanes = 1;
  buffer->info.bmiHeader.biBitCount = 32;
  buffer->info.bmiHeader.biCompression = BI_RGB;

  int guardPixel = 4;
  int bitmapMemorySize = (buffer->width * buffer->height + guardPixel) * bytePerPixel;
  buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

}

static void Win32BufferToWindow(HDC deviceContext, win32_offscreen_buffer* buffer, int windowWidth, int windowHeight)
{
  //Note: not stretching for debug purpose!
  //TODO: switch to aspect ratio in the future!

  buffer->offSet.x = 0.5f * (float)(windowWidth - buffer->width);
  buffer->offSet.y = 0.5f * (float)(windowHeight - buffer->height);

  StretchDIBits(deviceContext, (int)buffer->offSet.x, (int)buffer->offSet.y, buffer->width, buffer->height, 0, 0, buffer->width, buffer->height, buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

void Win32InitDSound(HWND window, int32_t samplesPerSecond, int32_t bufferSize)
{
  HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");
  if (dSoundLibrary)
  {
    direct_sound_create* DirectSoundCreate =
      (direct_sound_create*)GetProcAddress(dSoundLibrary,
                                           "DirectSoundCreate");
    LPDIRECTSOUND directSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
    {

      WAVEFORMATEX waveFormat;
      waveFormat.wFormatTag = WAVE_FORMAT_PCM;
      waveFormat.nChannels = 2;
      waveFormat.wBitsPerSample = 16;
      waveFormat.nSamplesPerSec = samplesPerSecond;
      waveFormat.nBlockAlign =
        (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
      waveFormat.nAvgBytesPerSec =
        waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
      waveFormat.cbSize = 0;

      if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
      {

        DSBUFFERDESC bufferDescription = {};
        bufferDescription.dwSize = sizeof(bufferDescription);
        bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        LPDIRECTSOUNDBUFFER primaryBuffer;
        if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription,
                                                     &primaryBuffer, 0)))
        {
          if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
          {
          }
          else
          {
            // TODO: Diagnostic
          }
        }
        else
        {
          // TODO: Diagnostic
        }
      }

      DSBUFFERDESC bufferDescription = {};
      bufferDescription.dwSize = sizeof(bufferDescription);
      bufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
      bufferDescription.dwBufferBytes = bufferSize;
      bufferDescription.lpwfxFormat = &waveFormat;

      if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &g_secondaryBuffer, 0)))
      {
      }
    }
    else
    {
      // TODO: Diagnostic
    }
  }
  else
  {
    // TODO: Diagnostic
  }
}

LRESULT CALLBACK MainWindowCallback(HWND window, UINT message, WPARAM wParam,
                                    LPARAM lParam)
{
  LRESULT result = 0;

  ASSERT(message != WM_KEYDOWN);
  ASSERT(message != WM_KEYUP);
  ASSERT(message != WM_SYSKEYDOWN);
  ASSERT(message != WM_SYSKEYUP);

  switch (message)
  {
    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
    }
    break;
    case WM_SETCURSOR:
    {
      if (g_IsCursorDisplay)
      {
        SetCursor(g_Cursor);
      }
    }
    break;
    case WM_DESTROY:
    {
      OutputDebugStringA("WM_DESTROY\n");
    }
    break;
    case WM_ACTIVATEAPP:
    {
      if (wParam == TRUE)
      {
        SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);
      }
      else
      {
        SetLayeredWindowAttributes(window, RGB(0, 0, 0), 128, LWA_ALPHA);
      }
    }
    break;
    case WM_CLOSE:
    {
      PostQuitMessage(0);
    }
    break;
    case WM_PAINT:
    {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(window, &paint);
      int x = paint.rcPaint.left;
      int y = paint.rcPaint.top;
      int width = paint.rcPaint.right - paint.rcPaint.left;
      int height = paint.rcPaint.bottom - paint.rcPaint.top;
      PatBlt(deviceContext, 0, 0, width, height, WHITENESS);

      EndPaint(window, &paint);
    }
    break;
    default:
    {
      result = DefWindowProc(window, message, wParam, lParam);
    }
    break;
  }
  return result;
}

void Win32ClearBuffer(win32_sound_output* buffer)
{
  void* region1;
  DWORD region1Size;
  void* region2;
  DWORD region2Size;
  // TODO: is it appropriate to lock entire buffer? if so should we use dwFlags or dwBytes
  if (SUCCEEDED(g_secondaryBuffer->Lock(0, buffer->secondaryBufferSize,
                                        &region1, &region1Size, &region2,
                                        &region2Size, DSBLOCK_ENTIREBUFFER)))
  {
    uint8_t* sampleOut = (uint8_t*)region1;
    for (DWORD i = 0; i < region1Size; ++i)
    {
      *sampleOut++ = 0;
    }

    sampleOut = (uint8_t*)region2;
    for (DWORD i = 0; i < region2Size; ++i)
    {
      *sampleOut++ = 0;
    }

    g_secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}

void Win32FillSoundBuffer(win32_sound_output* soundOutput, DWORD writePosition,
                          DWORD bytesWrite, game_sound_output* sourceBuffer)
{

  void* region1;
  DWORD region1Size;
  void* region2;
  DWORD region2Size;
  if (SUCCEEDED(g_secondaryBuffer->Lock(writePosition, bytesWrite, &region1,
                                        &region1Size, &region2, &region2Size,
                                        0)))
  {
    int16_t* sampleOut = (int16_t*)region1;
    int16_t* source = sourceBuffer->samples;
    DWORD region1BlockCount = region1Size / soundOutput->bytesPerBlock;
    for (DWORD i = 0; i < region1BlockCount; ++i)
    {
      *sampleOut++ = *source++;
      *sampleOut++ = *source++;

      ++soundOutput->blockIndex;
    }

    DWORD region2BlockCount = region2Size / soundOutput->bytesPerBlock;
    sampleOut = (int16_t*)region2;
    for (DWORD i = 0; i < region2BlockCount; ++i)
    {
      *sampleOut++ = *source++;
      *sampleOut++ = *source++;

      ++soundOutput->blockIndex;
    }
    g_secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}

#if INTERNAL
//TODO: Record to file for now. Change to memory or file maping if needed!

static void Win32StartRecord(win32_state* recordState)
{
  recordState->recordHandle = CreateFileA(recordState->fileFullPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  SetFilePointer(recordState->recordHandle, 0, 0, FILE_BEGIN);
  recordState->isRecording = true;
  recordState->recordIndex = 0;

  ASSERT(recordState->recordHandle != INVALID_HANDLE_VALUE);
}

static void Win32RecordInput(game_input input, win32_state* recordState)
{
  ASSERT(recordState->isPlaying == false);
  ASSERT(recordState->recordHandle != INVALID_HANDLE_VALUE);

  DWORD bytesWrite;
  if (recordState->recordIndex > 0)
  {
    WriteFile(recordState->recordHandle, &input, sizeof(input), &bytesWrite, 0);
  }
  else
  {
    //TODO: write to disk if viable
    memcpy(recordState->tempGameMemory, recordState->gameMemory, recordState->memorySize);
  }
  recordState->recordIndex++;

  ASSERT(bytesWrite > 0);
}

static void Win32StopRecord(win32_state* recordState)
{
  CloseHandle(recordState->recordHandle);
  recordState->isRecording = false;
}

static void Win32StartPlayback(win32_state* recordState)
{
  recordState->playHandle = CreateFileA(recordState->fileFullPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

  ASSERT(recordState->playHandle != INVALID_HANDLE_VALUE);

  recordState->isPlaying = true;
  recordState->playIndex = 0;

}

static void Win32StopPlayback(game_input* input, win32_state* recordState)
{
  CloseHandle(recordState->playHandle);
  recordState->isPlaying = false;

  //NOTE: clear input when finish playback
  *input = {};
}

static void Win32Playback(game_input* input, win32_state* recordState)
{
  ASSERT(recordState->isRecording == false);

  DWORD bytesRead;
  if (recordState->playIndex > 0)
  {
    if (ReadFile(recordState->playHandle, input, sizeof(game_input), &bytesRead, 0))
    {
      if (bytesRead == 0)
      {
        recordState->playIndex = 0;
        SetFilePointer(recordState->playHandle, 0, 0, FILE_BEGIN);
      }
      else
      {
        recordState->playIndex++;
      }
    }
  }
  else
  {
    //TODO: write to disk if viable
    memcpy(recordState->gameMemory, recordState->tempGameMemory, recordState->memorySize);
    recordState->playIndex++;
  }
}
#endif

static void Win32ProcessKeyboardInput(button_state* button, bool isDown)
{
  ASSERT(button->isDown != isDown);
  button->isDown = isDown;
  button->halfTransitionCount++;
}

static void ToggleFullScreen(HWND window, WINDOWPLACEMENT* windowPlacement)
{
  //Note: this method is according to Raymond Chen prescription.
  DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
  if (dwStyle & WS_OVERLAPPEDWINDOW) {
    MONITORINFO monitorInfo = { sizeof(monitorInfo) };
    if (GetWindowPlacement(window, windowPlacement) &&
        GetMonitorInfo(MonitorFromWindow(window,
                                         MONITOR_DEFAULTTOPRIMARY), &monitorInfo)) {
      SetWindowLong(window, GWL_STYLE,
                    dwStyle & ~WS_OVERLAPPEDWINDOW);
      SetWindowPos(window, HWND_TOP,
                   monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                   monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                   monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
  }
  else {
    SetWindowLong(window, GWL_STYLE,
                  dwStyle | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(window, windowPlacement);
    SetWindowPos(window, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

static void Win32ProcessPendingMessages(game_input* input, win32_state* win32State)
{
  MSG msg;
  while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
    switch (msg.message)
    {
      case WM_QUIT:
      {
        g_running = false;
      }
      break;

      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      case WM_KEYUP:
      {
        bool wasDown = ((msg.lParam & (1 << 30)) != 0);
        bool isDown = ((msg.lParam & (1 << 31)) == 0);

        if (isDown != wasDown)
        {
          if (msg.wParam == 'W')
          {
            Win32ProcessKeyboardInput(&input->up, isDown);
          }
          if (msg.wParam == 'S')
          {
            Win32ProcessKeyboardInput(&input->down, isDown);
          }
          if (msg.wParam == 'A')
          {
            Win32ProcessKeyboardInput(&input->left, isDown);
          }
          if (msg.wParam == 'D')
          {
            Win32ProcessKeyboardInput(&input->right, isDown);
          }
          if (msg.wParam == VK_ESCAPE)
          {
            Win32ProcessKeyboardInput(&input->escape, isDown);
          }
          if (msg.wParam == VK_SPACE)
          {
            Win32ProcessKeyboardInput(&input->space, isDown);
          }
          if (msg.wParam == VK_F1)
          {
            Win32ProcessKeyboardInput(&input->f1, isDown);
          }
          if (msg.wParam == VK_F3)
          {
            Win32ProcessKeyboardInput(&input->f3, isDown);
          }

          if (msg.wParam == 'K' && isDown)
          {
            if (!win32State->isRecording)
            {
              if (win32State->isPlaying)
              {
                Win32StopPlayback(input, win32State);
              }
              Win32StartRecord(win32State);
            }
            else
            {
              Win32StopRecord(win32State);
            }
          }

          if (msg.wParam == 'L' && isDown)
          {
            if (!win32State->isPlaying)
            {
              if (win32State->isRecording)
              {
                Win32StopRecord(win32State);
              }
              Win32StartPlayback(win32State);
            }
            else
            {
              Win32StopPlayback(input, win32State);
            }
          }

          if (msg.wParam == 'P' && isDown)
          {
            g_pause = !g_pause;
          }

          if (msg.wParam == VK_F3 && isDown)
          {
            g_audioSyncDisplay = !g_audioSyncDisplay;
          }

          if (isDown)
          {
            bool32 altKey = (msg.lParam & (1 << 29));
            if (altKey && msg.wParam == VK_F4)
            {
              PostQuitMessage(0);
            }
            if (altKey && msg.wParam == VK_RETURN)
            {
              ToggleFullScreen(msg.hwnd, &g_wpPrev);
            }

          }
        }
      }
      break;
      default:
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  // TODO: Controller Support ?
  //   Win32LoadXInput();
  //   for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
  //   {
  //     XINPUT_STATE state;
  //     if (XInputGetState(i, &state) == ERROR_SUCCESS)
  //     {
  //       XINPUT_GAMEPAD* pad = &state.Gamepad;

  //       bool up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
  //       bool down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
  //       bool left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
  //       bool right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
  //       bool start = pad->wButtons & XINPUT_GAMEPAD_START;
  //       bool back = pad->wButtons & XINPUT_GAMEPAD_BACK;
  //       bool leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
  //       bool rightShoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
  //       bool a = pad->wButtons & XINPUT_GAMEPAD_A;
  //       bool b = pad->wButtons & XINPUT_GAMEPAD_B;
  //       bool x = pad->wButtons & XINPUT_GAMEPAD_X;
  //       bool y = pad->wButtons & XINPUT_GAMEPAD_Y;

  //       int16_t stickX = pad->sThumbLX;
  //       int16_t stickY = pad->sThumbLY;
  //     }
  //     else
  //     {
  //       // TODO: Controller not available
  //     }
  //   }
}

inline LARGE_INTEGER Win32GetWallClock()
{
  LARGE_INTEGER timer;
  QueryPerformanceCounter(&timer);

  return timer;
}

static void HandleDebugCycleCounter(game_memory* memory)
{
#if INTERNAL

  OutputDebugStringA("DEBUG CYCLE COUNTS:\n");
  for (int i = 0; i < ARRAY_COUNT(memory->counter); ++i)
  {
    debug_cycle_counter* counter = memory->counter + i;
    if (counter->hitCount)
    {
      char buf[256];
      _snprintf_s(buf, sizeof(buf), "%d: %lldcy %uh %lldcy/h\n", i, counter->cycleCount, counter->hitCount, counter->cycleCount / counter->hitCount);
      OutputDebugStringA(buf);

      counter->cycleCount = 0;
      counter->hitCount = 0;
    }
  }

#endif
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER before, LARGE_INTEGER after, LARGE_INTEGER frequency)
{
  return (float)(after.QuadPart - before.QuadPart) / frequency.QuadPart;
}

#if INTERNAL
void DebugDrawVerticalLine(game_offscreen_buffer* screen, int pos, int top, int bottom, uint32_t color)
{
  pos = (pos >= screen->width) ? pos % (screen->width) : pos;
  for (int i = top; i < bottom; ++i)
  {
    uint32_t* buffer = (uint32_t*)screen->memory;
    buffer[i * screen->width + pos] = color;
  }
}

void DebugAudioSync(game_offscreen_buffer* screen, win32_sound_output soundBuffer,
                    debug_audio_marker marker)
{
  // TODO: Output Info
  int cellHeight = 32;
  int padX = 16;
  int padY = 16;
  int width = screen->width - padX * 2;
  float c = (float)width / soundBuffer.secondaryBufferSize;
  uint32_t playColor = 0xFFFFFF;
  uint32_t writeColor = 0xFF0000;
  uint32_t boundaryColor = 0x00FFFF;
  uint32_t expectedColor = 0xFFFF00;

  int top = padY;
  for (int i = 0; i < marker.playCursorCount; ++i)
  {
    int playCursorPos = padX + (int)(c * marker.playCursors[i]);
    DebugDrawVerticalLine(screen, playCursorPos, top, top + cellHeight, playColor);
  }
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.writeCursor),
                        top, top + cellHeight, writeColor);

  top += cellHeight;
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.flipPlayCursor), top, top + cellHeight, playColor);
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.flipWriteCursor), top, top + cellHeight, writeColor);
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.nextFrameBoundary), top, top + cellHeight, boundaryColor);

  top += cellHeight;
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.expectedWritePosition), top, top + cellHeight, expectedColor);
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.expectedTargetCursor), top, top + cellHeight, expectedColor);
}
#endif

void CatStrings(char* sourceA, size_t sourceACount, char* sourceB, size_t sourceBCount, char* dest, size_t* destCount)
{
  for (size_t i = 0; i < sourceACount; ++i)
  {
    *(dest++) = sourceA[i];
    *destCount++;
  }

  for (size_t i = 0; i < sourceBCount; ++i)
  {
    *(dest++) = sourceB[i];
    *destCount++;
  }
  *dest = 0;
}

size_t StringLength(char* str)
{
  size_t result = 0;
  while (*str++)
  {
    result++;
  }
  return result;
}

#include <intrin.h>

struct win32_thread_info
{
  i32 logicalThreadIndex;
  platform_work_queue* queue;
};

PLATFORM_ADD_WORK_ENTRY(Win32AddWorkEntry)
{
  ASSERT(queue->entryIndex < ARRAY_COUNT(queue->entries));

  queue->entries[queue->entryLastIndex].callback = callback;
  queue->entries[queue->entryLastIndex].data = data;
  _WriteBarrier();
  _mm_sfence();
  queue->entryLastIndex = (queue->entryLastIndex + 1) % ARRAY_COUNT(queue->entries);
  ++queue->entryInProgress;
  ReleaseSemaphore(queue->semaphoreHandle, 1, 0);
}

bool HasWorkInQueue(platform_work_queue* queue)
{
  return (queue->entryInProgress > 0);
}

#define WORK_ENTRY_PROC(name) (void (*)(void*))name

void PrintString(void* str)
{
  OutputDebugString((char*)str);
}

void PushString(platform_work_queue* queue, char* str)
{
  Win32AddWorkEntry(queue, (work_entry_callback*)OutputDebugString, str);
}

bool DoWorkQueueWork(platform_work_queue* queue, i32 logicalThreadIndex)
{
  //NOTE: return true when all the work has started
  bool isWorksAllStarted = false;
  work_queue_entry* entries = queue->entries;
  i32 orgIndex = queue->entryIndex;
  i32 nextIndex = (orgIndex + 1) % ARRAY_COUNT(queue->entries);
  if (orgIndex != queue->entryLastIndex)
  {
    i32 index = InterlockedCompareExchange((LONG volatile*)&queue->entryIndex, nextIndex, orgIndex);

    if (orgIndex == index)
    {
      _ReadBarrier();
      entries[index].callback(entries[index].data);
      entries[index].currentThreadIndex = logicalThreadIndex;

      InterlockedDecrement((LONG volatile*)&queue->entryInProgress);
    }
  }
  else
  {
    isWorksAllStarted = true;
  }
  return isWorksAllStarted;
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
  win32_thread_info* info = (win32_thread_info*)lpParam;
  platform_work_queue* queue = info->queue;
  for (;;)
  {
    if (DoWorkQueueWork(queue, info->logicalThreadIndex))
    {
      WaitForSingleObjectEx(queue->semaphoreHandle, INFINITE, FALSE);
    }
  }
}

void Win32CompleteAllWork(platform_work_queue* queue)
{
  while (HasWorkInQueue(queue))
  {
    DoWorkQueueWork(queue, -1);
  }

  queue->entryIndex = 0;
  queue->entryLastIndex = 0;

}

INT __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine,
                      INT cmdShow)
{
  win32_thread_info info[8];
  i32 threadCount = ARRAY_COUNT(info);

  i32 initialCount = 0;
  platform_work_queue queue = {};
  queue.semaphoreHandle = CreateSemaphoreEx(0, initialCount, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS);


  for (i32 i = 0; i < threadCount; ++i)
  {
    info[i].logicalThreadIndex = i;
    info[i].queue = &queue;

    DWORD threadID;
    HANDLE threadHandle = CreateThread(0, 0, ThreadProc, &info[i], 0, &threadID);
    CloseHandle(threadHandle);
  }

  //Note: Dont use MAX_PATH in release code 'cause it's wrong
  char gameStem[MAX_PATH] = {};
  char* onePastLastSlash = gameStem;
  GetModuleFileName(instance, gameStem, MAX_PATH);
  for (char* scan = gameStem; *scan; scan++)
  {
    if (*scan == '\\')
    {
      onePastLastSlash = scan + 1;
    }
  }
  size_t gameStemCount = onePastLastSlash - gameStem;

  char gameDLLName[] = "game.dll";
  char gameDllFullPath[MAX_PATH] = {};
  size_t gameFullPathCount = 0;
  CatStrings(gameStem, gameStemCount, gameDLLName, StringLength(gameDLLName), gameDllFullPath, &gameFullPathCount);

  char tempGameDllName[] = "game_temp.dll";
  char tempGameDllFullPath[MAX_PATH] = {};
  size_t tempGameDllFullPathCount = 0;
  CatStrings(gameStem, onePastLastSlash - gameStem, tempGameDllName, StringLength(tempGameDllName), tempGameDllFullPath, &tempGameDllFullPathCount);

  // NOTE: Increase scheduler granularity
  UINT msMinimumTimeRes = 1;
  bool isSchedulerSet = (timeBeginPeriod(msMinimumTimeRes) == TIMERR_NOERROR);

  WNDCLASSA windowClass = {};
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = MainWindowCallback;
  windowClass.hInstance = instance;
  // windowClass.hIcon = ;
  windowClass.lpszClassName = "MainClass";
  g_Cursor = LoadCursor(0, IDC_ARROW);
  g_IsCursorDisplay = true;

  if (RegisterClassA(&windowClass))
  {
    HWND window = CreateWindowExA(WS_EX_OVERLAPPEDWINDOW,
                                  windowClass.lpszClassName, "Main",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 0, 0, instance, 0);

    Win32ResizeDIBSection(&g_backBuffer, 1024, 576);

    if (window)
    {
      g_running = true;

      int monitorRefreshHz = 60;
      HDC refreshDC = GetDC(window);
      int win32MonitorRefreshHz = GetDeviceCaps(refreshDC, VREFRESH);
      ReleaseDC(window, refreshDC);
      if (win32MonitorRefreshHz > 1)
      {
        monitorRefreshHz = win32MonitorRefreshHz;
      }
      float gameUpdateHz = (monitorRefreshHz / 2.0f);
      float secondsPerUpdate = 1.0f / gameUpdateHz;
      win32_sound_output soundOutput = {};
      soundOutput.blockIndex = 0;
      soundOutput.bytesPerBlock = sizeof(int16_t) * 2;
      soundOutput.blocksPerSecond = 44100;
      soundOutput.secondaryBufferSize = soundOutput.blocksPerSecond * soundOutput.bytesPerBlock;
      soundOutput.safetyBytes = (int)((float)(soundOutput.blocksPerSecond * soundOutput.bytesPerBlock / gameUpdateHz) / 3.0f);
      Win32InitDSound(window, soundOutput.blocksPerSecond,
                      soundOutput.secondaryBufferSize);
      Win32ClearBuffer(&soundOutput);
      g_secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      int16_t* samples = (int16_t*)VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if INTERNAL
      LPVOID baseAddress = (LPVOID)TERABYTES((uint64_t)2);
#else
      LPVOID baseAddress = 0;
#endif
      // Static Memory allocation at start up
      game_memory gameMemory = {};
      gameMemory.permanentStorageSize = MEGABYTES(512);
      gameMemory.transientStorageSize = MEGABYTES(32);
      gameMemory.DEBUGPlatformFreeMemory = DEBUGPlatformFreeMemory;
      gameMemory.DEBUGPlatformReadFile = DEBUGPlatformReadFile;
      gameMemory.DEBUGPlatformWriteFile = DEBUGPlatformWriteFile;
      gameMemory.workQueue = &queue;

      gameMemory.PlatformAddWorkEntry = (platform_add_work_entry*)Win32AddWorkEntry;
      gameMemory.PlatformCompleteAllWork = (platform_complete_all_work*)Win32CompleteAllWork;

      uint64_t totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;

      win32_state recordState = {};

      //TODO: Try MEM_LARGE_PAGES to increase performace. call AdjustTokenPrivileges on winxp 
      recordState.gameMemory = VirtualAlloc(baseAddress, (size_t)totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
      recordState.tempGameMemory = VirtualAlloc((LPVOID)((u64)baseAddress + totalSize), (size_t)totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

      gameMemory.permanentStorage = (uint8_t*)recordState.gameMemory;
      gameMemory.transientStorage = (uint8_t*)gameMemory.permanentStorage + gameMemory.permanentStorageSize;

      if (gameMemory.permanentStorage && gameMemory.transientStorage && samples)
      {
        recordState.memorySize = (DWORD)totalSize;
        ASSERT(recordState.memorySize == totalSize);

        float secondsElapsed = 0.0f;
        LARGE_INTEGER beginTimer = Win32GetWallClock();
        LARGE_INTEGER frequency = {};
        QueryPerformanceFrequency(&frequency);

        bool soundIsValid = false;

        win32_game_code game = Win32LoadGameCode(gameDllFullPath, tempGameDllFullPath);

        game_input gameInput[2] = {};
        game_input* input = &gameInput[0];
        game_input* lastInput = &gameInput[1];

        char recordFile[] = "record.gi";
        char recordFileFullPath[MAX_PATH];
        size_t recordFileCount = 0;
        CatStrings(gameStem, gameStemCount, recordFile, sizeof(recordFile), recordFileFullPath, &recordFileCount);

        recordState.fileFullPath = recordFileFullPath;


#if INTERNAL
        debug_audio_marker marker = {};
        DWORD temp[10] = { 0 };
        marker.playCursorCount = 10;
        marker.playCursors = temp;
        int debugPlayCursorsIndex = 0;
#endif

        while (g_running)
        {
          FILETIME newFileTime = Win32GetLastFileTime(gameDllFullPath);
          if (CompareFileTime(&newFileTime, &game.lastWriteTime) != 0)
          {
            Win32UnloadGameCode(&game);
            game = Win32LoadGameCode(gameDllFullPath, tempGameDllFullPath);
          }

          *input = {};
          int buttonsCount = ARRAY_COUNT(input->buttons);
          for (int i = 0; i < buttonsCount; ++i)
          {
            // NOTE: reserve the button state to next frame
            input->buttons[i].isDown = lastInput->buttons[i].isDown;
          }
          Win32ProcessPendingMessages(input, &recordState);

          //TODO: Hacky thing to get going!
          POINT mousePoint;
          GetCursorPos(&mousePoint);
          ScreenToClient(window, &mousePoint);

          //NOTE: remap the origin to bottom half of the draw buffer
          input->mouseX = mousePoint.x - (i32)g_backBuffer.offSet.x;
          input->mouseY = (i32)(g_backBuffer.offSet.y + g_backBuffer.height) - mousePoint.y;


          input->mouseButtonState[0] = GetKeyState(VK_LBUTTON) & (1 << 15);
          input->mouseButtonState[1] = GetKeyState(VK_RBUTTON) & (1 << 15);
          input->mouseButtonState[2] = GetKeyState(VK_MBUTTON) & (1 << 15);
          input->mouseButtonState[3] = GetKeyState(VK_XBUTTON1) & (1 << 15);
          input->mouseButtonState[4] = GetKeyState(VK_XBUTTON2) & (1 << 15);

          if (window == GetActiveWindow())
          {
            RECT wRect;
            GetWindowRect(window, &wRect);
            int mX = (wRect.left + wRect.right) / 2;
            int mY = (wRect.top + wRect.bottom) / 2;
            SetCursorPos(mX, mY);

            POINT midPoint = { mX, mY };
            ScreenToClient(window, &midPoint);

            input->dMouseX = mousePoint.x - midPoint.x;
            input->dMouseY = -(mousePoint.y - midPoint.y);

            // char buf[256];
            // _snprintf_s(buf, sizeof(buf), "MouseX: %i, MouseY: %i", input->dMouseX, input->dMouseY);
            // OutputDebugStringA(buf);

            ShowCursor(false);
          }

          // Note: Update the game in fixed interval
          input->dt = secondsPerUpdate;

          if (recordState.isRecording)
          {
            Win32RecordInput(*input, &recordState);
          }
          if (recordState.isPlaying)
          {
            Win32Playback(input, &recordState);
          }

          if (!g_pause)
          {
            game_offscreen_buffer buffer = {};
            buffer.memory = g_backBuffer.memory;
            buffer.width = g_backBuffer.width;
            buffer.height = g_backBuffer.height;

            if (game.UpdateAndRender)
            {
              game.UpdateAndRender(&gameMemory, &buffer, *input);
              HandleDebugCycleCounter(&gameMemory);
            }


            // TODO: Continue when has more precise profiling tool!
            DWORD writeCursor;
            DWORD playCursor;
            if (SUCCEEDED(g_secondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
            {
              if (!soundIsValid)
              {
                soundOutput.blockIndex = writeCursor / soundOutput.bytesPerBlock;
                soundIsValid = true;
              }

              DWORD writePosition = (soundOutput.blockIndex * soundOutput.bytesPerBlock) %
                soundOutput.secondaryBufferSize;

              DWORD bytesPerFrame = (DWORD)((float)(soundOutput.blocksPerSecond * soundOutput.bytesPerBlock) / gameUpdateHz);

              float secondsSinceFlip = Win32GetSecondsElapsed(beginTimer, Win32GetWallClock(), frequency);

              DWORD nextFrameBoundary = playCursor + bytesPerFrame;
              DWORD bytesSinceFlip = (DWORD)(secondsSinceFlip * (soundOutput.blocksPerSecond * soundOutput.bytesPerBlock));

              if (nextFrameBoundary < bytesSinceFlip)
              {
                nextFrameBoundary += soundOutput.secondaryBufferSize;
              }
              nextFrameBoundary -= bytesSinceFlip;


              DWORD safetyWriteCursor = writeCursor;
              if (safetyWriteCursor < playCursor)
              {
                safetyWriteCursor += soundOutput.secondaryBufferSize;
              }
              safetyWriteCursor += soundOutput.safetyBytes;
              ASSERT(safetyWriteCursor > playCursor);


              bool audioIsSync = safetyWriteCursor < nextFrameBoundary;

              DWORD targetCursor = 0;
              if (audioIsSync)
              {
                targetCursor = nextFrameBoundary + bytesPerFrame;
              }
              else
              {
                targetCursor = safetyWriteCursor + bytesPerFrame;
              }
              targetCursor %= soundOutput.secondaryBufferSize;

              DWORD bytesWrite = 0;
              if (writePosition > targetCursor)
              {
                bytesWrite = soundOutput.secondaryBufferSize - writePosition;
                bytesWrite += targetCursor;
              }
              else
              {
                bytesWrite = targetCursor - writePosition;
              }

              game_sound_output soundBuffer = {};
              soundBuffer.bytesPerSample = soundOutput.bytesPerBlock / 2;
              soundBuffer.sampleCount = bytesWrite / soundBuffer.bytesPerSample;
              soundBuffer.samples = samples;

              game.OutputSound(&gameMemory, &soundBuffer);

              Win32FillSoundBuffer(&soundOutput, writePosition,
                                   bytesWrite, &soundBuffer);

#if INTERNAL
              marker.expectedWritePosition = writePosition;
              marker.expectedTargetCursor = targetCursor;
              marker.nextFrameBoundary = nextFrameBoundary;
              marker.playCursors[debugPlayCursorsIndex] = playCursor;
              marker.writeCursor = writeCursor;
#endif
            }
            else
            {
              soundIsValid = false;
            }


            secondsElapsed = Win32GetSecondsElapsed(beginTimer, Win32GetWallClock(), frequency);
            if (secondsElapsed < secondsPerUpdate)
            {
              if (isSchedulerSet)
              {
                DWORD msHalt = (DWORD)(1000.0f * (secondsPerUpdate - secondsElapsed));
                if (msHalt > 0)
                {
                  Sleep(msHalt);
                }
              }

              secondsElapsed = Win32GetSecondsElapsed(beginTimer, Win32GetWallClock(), frequency);

              if (secondsElapsed < secondsPerUpdate)
              {
                // TODO: Sleep Missed!
              }

              while (secondsElapsed < secondsPerUpdate)
              {
                // TODO: Cant set timer resolution
                secondsElapsed = Win32GetSecondsElapsed(beginTimer, Win32GetWallClock(), frequency);
              }
            }
            else
            {
              // TODO: Missed a frame
              // TODO: Logging
            }
            beginTimer = Win32GetWallClock();

#if INTERNAL
            {
              // TODO: Improve Debug Info and Drawing Routine 
              DWORD writeCursorWhenFlip;
              DWORD playCursorWhenFlip;
              g_secondaryBuffer->GetCurrentPosition(&playCursorWhenFlip, &writeCursorWhenFlip);
              marker.flipPlayCursor = playCursorWhenFlip;
              marker.flipWriteCursor = writeCursorWhenFlip;
            }
            if (g_audioSyncDisplay)
            {
              DebugAudioSync(&buffer, soundOutput, marker);
            }
#endif

            win32_window_dimension wd = Win32GetWindowDimension(window);
            HDC deviceContext = GetDC(window);
            Win32BufferToWindow(deviceContext, &g_backBuffer, wd.width, wd.height);
            ReleaseDC(window, deviceContext);

#if INTERNAL
            ASSERT(debugPlayCursorsIndex < marker.playCursorCount);
            debugPlayCursorsIndex = (debugPlayCursorsIndex + 1) % marker.playCursorCount;
#endif
            game_input* tempInput = input;
            input = lastInput;
            lastInput = tempInput;
          }
        }
      }
      else
      {
        // TODO: Logging
      }
    }
    else
    {
      // TODO: Logging
    }
  }
  else
  {
    // TODO: Logging
  }
  return 0;
}
