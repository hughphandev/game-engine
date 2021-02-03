
#include "game.h"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>


#include <math.h>
#include <stdint.h>

#include "win32_platform.h"

// TODO: Global for now
static bool g_running;
static bool g_pause;
static win32_offscreen_buffer g_backBuffer;
static LPDIRECTSOUNDBUFFER g_secondaryBuffer;

inline uint32_t SafeTruncateUInt64(uint64_t value)
{
  // TODO: Define maximum value
  ASSERT(value < 0xFFFFFFFF);
  return (uint32_t)value;
}

inline FILETIME Win32GetLastFileTime(char* fileName)
{
  FILETIME result = {};

  WIN32_FIND_DATA findData = {};
  HANDLE fileHandle = FindFirstFile(fileName, &findData);
  if (fileHandle != INVALID_HANDLE_VALUE)
  {
    result = findData.ftLastWriteTime;
    FindClose(fileHandle);
  }

  return result;
}

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

static void Win32LoadXInput()
{
  HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (!xInputLibrary)
  {
    xInputLibrary = LoadLibraryA("xinput1_3.dll");
  }
  if (xInputLibrary)
  {
    XInputGetState =
      (x_input_get_state*)GetProcAddress(xInputLibrary, "XInputGetState");
    XInputSetState =
      (x_input_set_state*)GetProcAddress(xInputLibrary, "XInputSetState");
  }
}

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
          //TODO: File read sucessfully
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

bool DEBUGPlatformWriteFile(char* fileName, uint32_t memorySize, void* memory)
{
  bool result = false;
  HANDLE fileHandle = CreateFileA(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (fileHandle != INVALID_HANDLE_VALUE)
  {
    DWORD bytesWrite;
    if (WriteFile(fileHandle, memory, memorySize, &bytesWrite, 0))
    {
      result = (bytesWrite == memorySize);
      //TODO: File write sucessfully
    }
    else
    {
      //TODO: Logging
    }
    FindClose(fileHandle);
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

static void Win32ResizeDIBSection(win32_offscreen_buffer* buffer, int width,
                                  int height)
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
  buffer->info.bmiHeader.biHeight = -buffer->height;
  buffer->info.bmiHeader.biPlanes = 1;
  buffer->info.bmiHeader.biBitCount = 32;
  buffer->info.bmiHeader.biCompression = BI_RGB;

  int bitmapMemorySize = buffer->width * buffer->height * bytePerPixel;
  buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);

  buffer->pitch = buffer->width * bytePerPixel;
}

static void Win32BufferToWindow(HDC deviceContext,
                                win32_offscreen_buffer* buffer, int x, int y,
                                int windowWidth, int windowHeight)
{
  StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight, 0, 0,
                buffer->width, buffer->height, buffer->memory, &buffer->info,
                DIB_RGB_COLORS, SRCCOPY);
}

void Win32InitDSound(HWND window, int32_t samplesPerSecond,
                     int32_t bufferSize)
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

      if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription,
                                                   &g_secondaryBuffer, 0)))
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

  switch (message)
  {
    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
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
      Win32BufferToWindow(deviceContext, &g_backBuffer, 0, 0, width, height);

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
    DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
    for (DWORD i = 0; i < region1SampleCount; ++i)
    {
      *sampleOut++ = *source++;
      *sampleOut++ = *source++;

      ++soundOutput->runningSampleIndex;
    }

    DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
    sampleOut = (int16_t*)region2;
    for (DWORD i = 0; i < region2SampleCount; ++i)
    {
      *sampleOut++ = *source++;
      *sampleOut++ = *source++;

      ++soundOutput->runningSampleIndex;
    }
    g_secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}

#if INTERNAL
static void Win32StartRecord(win32_state* recordState)
{
  recordState->recordHandle = CreateFileA(recordState->fileFullPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  SetFilePointer(recordState->recordHandle, 0, 0, FILE_BEGIN);
  recordState->isRecording = true;

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
    WriteFile(recordState->recordHandle, recordState->gameMemory, sizeof(game_state), &bytesWrite, 0);
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

static void Win32StopPlayback(win32_state* recordState)
{
  CloseHandle(recordState->playHandle);
  recordState->isPlaying = false;
}
#endif

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
        SetFilePointer(recordState->playHandle, 0, 0, FILE_BEGIN);
        Win32StopPlayback(recordState);
        Win32StartPlayback(recordState);
      }
      else
      {
        recordState->playIndex++;
      }
    }
  }
  else
  {
    if (ReadFile(recordState->playHandle, recordState->gameMemory, sizeof(game_state), &bytesRead, 0))
    {
      recordState->playIndex++;
    }
  }
}

static void Win32ProcessPendingMessages(game_input* input, win32_state* recordState)
{
  MSG msg;
  while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
    switch (msg.message)
    {
      // TODO: is it necessary to handle the WM_QUIT?
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
        input->keyCode = msg.wParam;
        input->wasDown = ((msg.lParam & (1 << 30)) != 0);
        input->isDown = ((msg.lParam & (1 << 31)) == 0);

        bool32 altKeyWasDown = (msg.lParam & (1 << 29));
        if (altKeyWasDown && msg.wParam == VK_F4)
        {
          g_running = false;
        }

        if (input->keyCode == 'P' && input->isDown)
        {
          g_pause = !g_pause;
        }

        if (input->keyCode == 'K' && input->isDown)
        {
          if (!recordState->isRecording)
          {
            Win32StartRecord(recordState);
          }
          else
          {
            Win32StopRecord(recordState);
          }
        }

        if (input->keyCode == 'L' && input->isDown)
        {
          if (!recordState->isPlaying)
          {
            Win32StartPlayback(recordState);
          }
          else
          {
            Win32StopPlayback(recordState);
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
  Win32LoadXInput();
  for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
  {
    XINPUT_STATE state;
    if (XInputGetState(i, &state) == ERROR_SUCCESS)
    {
      XINPUT_GAMEPAD* pad = &state.Gamepad;

      bool up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
      bool down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
      bool left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
      bool right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
      bool start = pad->wButtons & XINPUT_GAMEPAD_START;
      bool back = pad->wButtons & XINPUT_GAMEPAD_BACK;
      bool leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
      bool rightShoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
      bool a = pad->wButtons & XINPUT_GAMEPAD_A;
      bool b = pad->wButtons & XINPUT_GAMEPAD_B;
      bool x = pad->wButtons & XINPUT_GAMEPAD_X;
      bool y = pad->wButtons & XINPUT_GAMEPAD_Y;

      int16_t stickX = pad->sThumbLX;
      int16_t stickY = pad->sThumbLY;
    }
    else
    {
      // TODO: Controller not available
    }
  }
}

inline LARGE_INTEGER Win32GetWallClock()
{
  LARGE_INTEGER timer;
  QueryPerformanceCounter(&timer);

  return timer;
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER before, LARGE_INTEGER after, LARGE_INTEGER frequency)
{
  return (float)(after.QuadPart - before.QuadPart) / frequency.QuadPart;
}

#if INTERNAL
void DebugDrawVerticalLine(game_offscreen_buffer* screen, int pos, int top, int bottom, uint32_t color)
{
  pos %= screen->width;
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
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.flipPlayCursor),
                        top, top + cellHeight, playColor);
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.flipWriteCursor),
                        top, top + cellHeight, writeColor);
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.nextFrameBoundary),
                        top, top + cellHeight, boundaryColor);

  top += cellHeight;
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.expectedWritePosition),
                        top, top + cellHeight, expectedColor);
  DebugDrawVerticalLine(screen, padX + (int)(c * marker.expectedTargetCursor),
                        top, top + cellHeight, expectedColor);
}
#endif

size_t CatStrings(char* sourceA, size_t sourceACount, char* sourceB, size_t sourceBCount, char* dest, size_t destCount)
{
  if (destCount > sourceACount + sourceBCount)
  {
    for (size_t i = 0; i < sourceACount; ++i)
    {
      *(dest++) = sourceA[i];
    }

    for (size_t i = 0; i < sourceBCount; ++i)
    {
      *(dest++) = sourceB[i];
    }
    *dest = 0;
    return sourceACount + sourceBCount;
  }
  return 0;
}

#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz / 2)
INT __stdcall WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine,
                      INT cmdShow)
{
  //Note: Dont use MAX_PATH in release code 'cause it's wrong
  char gamePath[MAX_PATH] = {};
  char* onePastLastSlash = gamePath;
  GetModuleFileName(instance, gamePath, MAX_PATH);
  for (char* scan = gamePath; *scan; scan++)
  {
    if (*scan == '\\')
    {
      onePastLastSlash = scan + 1;
    }
  }

  char gameDLLName[] = "game.dll";
  char gameDllFullPath[MAX_PATH] = {};
  size_t gamePathCount = onePastLastSlash - gamePath;

  CatStrings(gamePath, gamePathCount, gameDLLName, sizeof(gameDLLName) - 1, gameDllFullPath, MAX_PATH);

  char tempGameDllName[] = "game_temp.dll";
  char tempGameDllFullPath[MAX_PATH] = {};

  CatStrings(gamePath, onePastLastSlash - gamePath, tempGameDllName, sizeof(tempGameDllName) - 1, tempGameDllFullPath, MAX_PATH);

  // NOTE: Increase scheduler granularity
  UINT msMinimumTimeRes = 1;
  bool isSchedulerSet = (timeBeginPeriod(msMinimumTimeRes) == TIMERR_NOERROR);

  WNDCLASSA windowClass = {};
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = MainWindowCallback;
  windowClass.hInstance = instance;
  // windowClass.hIcon = ;
  windowClass.lpszClassName = "MainClass";

  if (RegisterClassA(&windowClass))
  {
    HWND window = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED,
                                  windowClass.lpszClassName, "Main",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 0, 0, instance, 0);

    Win32ResizeDIBSection(&g_backBuffer, 1280, 720);

    if (window)
    {
      g_running = true;

      float secondsPerUpdate = 1.0f / GameUpdateHz;
      win32_sound_output soundOutput = {};
      soundOutput.runningSampleIndex = 0;
      soundOutput.bytesPerSample = sizeof(int16_t) * 2;
      soundOutput.samplesPerSecond = 48000;
      soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
      soundOutput.safetyBytes = (soundOutput.samplesPerSecond * soundOutput.bytesPerSample / GameUpdateHz) / 2;
      Win32InitDSound(window, soundOutput.samplesPerSecond,
                      soundOutput.secondaryBufferSize);
      Win32ClearBuffer(&soundOutput);
      g_secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      int16_t* samples = (int16_t*)VirtualAlloc(0, soundOutput.secondaryBufferSize,
                                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#if INTERNAL
      LPVOID baseAddress = (LPVOID)TERABYTES((uint64_t)2);
#else
      LPVOID baseAddress = 0;
#endif
      // Static Memory allocation at start up

      game_memory gameMemory = {};
      gameMemory.permanentStorageSize = MEGABYTES(64);
      gameMemory.transientStorageSize = GIGABYTES(1);
      gameMemory.DEBUGPlatformFreeMemory = DEBUGPlatformFreeMemory;
      gameMemory.DEBUGPlatformReadFile = DEBUGPlatformReadFile;
      gameMemory.DEBUGPlatformWriteFile = DEBUGPlatformWriteFile;

      uint64_t totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;

      win32_state recordState = {};

      recordState.gameMemory = VirtualAlloc(baseAddress, (size_t)totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

      gameMemory.permanentStorage = (uint8_t*)recordState.gameMemory;
      gameMemory.transientStorage = (uint8_t*)gameMemory.permanentStorage + gameMemory.permanentStorageSize;

      if (gameMemory.permanentStorage && gameMemory.transientStorage && samples)
      {
        recordState.memorySize = (DWORD)totalSize;
        ASSERT(recordState.memorySize == totalSize);

        float secondsElapsed = 0.0f;
        LARGE_INTEGER beginTimer = Win32GetWallClock();

        bool soundIsValid = false;

        LARGE_INTEGER frequency = {};
        QueryPerformanceFrequency(&frequency);

        win32_game_code game = Win32LoadGameCode(gameDllFullPath, tempGameDllFullPath);

        char recordFile[] = "record.gi";
        char recordFileFullPath[MAX_PATH];
        CatStrings(gamePath, gamePathCount, recordFile, sizeof(recordFile), recordFileFullPath, MAX_PATH);

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
          game_input input = {};
          Win32ProcessPendingMessages(&input, &recordState);
          // Note: Update the game in fixed interval
          input.elapsed = secondsElapsed;
          if (recordState.isRecording)
          {
            Win32RecordInput(input, &recordState);
          }
          else if (recordState.isPlaying)
          {
            Win32Playback(&input, &recordState);
          }

          if (!g_pause)
          {
            game_offscreen_buffer buffer = {};
            buffer.memory = g_backBuffer.memory;
            buffer.width = g_backBuffer.width;
            buffer.height = g_backBuffer.height;
            buffer.pitch = g_backBuffer.pitch;

            game.UpdateAndRender(&gameMemory, &buffer, input);

            // TODO: Not Tested yet. Properly buggy!
            DWORD writeCursor;
            DWORD playCursor;
            if (SUCCEEDED(g_secondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
            {
              if (!soundIsValid)
              {
                soundOutput.runningSampleIndex = writeCursor / soundOutput.bytesPerSample;
                soundIsValid = true;
              }

              DWORD writePosition = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
                soundOutput.secondaryBufferSize;

              DWORD bytesPerFrame = (soundOutput.samplesPerSecond * soundOutput.bytesPerSample) / GameUpdateHz;

              float secondsSinceFlip = Win32GetSecondsElapsed(beginTimer, Win32GetWallClock(), frequency);
              DWORD nextFrameBoundary = playCursor + bytesPerFrame;
              DWORD bytesSinceFlip = (DWORD)(secondsSinceFlip * (soundOutput.samplesPerSecond * soundOutput.bytesPerSample));
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
              soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
              soundBuffer.sampleCount = bytesWrite / soundOutput.bytesPerSample;
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
              DebugAudioSync(&buffer, soundOutput, marker);
            }
#endif

            win32_window_dimension wd = Win32GetWindowDimension(window);
            HDC deviceContext = GetDC(window);
            Win32BufferToWindow(deviceContext, &g_backBuffer, 0, 0, wd.width, wd.height);
            ReleaseDC(window, deviceContext);

#if INTERNAL
            {
              char textBuffer[255] = {};
              sprintf_s(textBuffer, "%.2fms\n", 1000.0f * secondsElapsed);
              OutputDebugString(textBuffer);
            }
            ASSERT(debugPlayCursorsIndex < marker.playCursorCount);
            debugPlayCursorsIndex = (debugPlayCursorsIndex + 1) % marker.playCursorCount;
#endif
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
