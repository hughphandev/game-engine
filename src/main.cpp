#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

struct win32_window_dimension
{
  int width;
  int height;
};

struct win32_offscreen_buffer
{
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int pitch;
};

typedef int32_t bool32;

#define X_INPUT_SET_STATE(name) DWORD name (DWORD dwUserIndex, XINPUT_VIBRATION *pVibrationdt)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}

#define X_INPUT_GET_STATE(name) DWORD name (DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

static x_input_set_state *XInputSetState_ = XInputSetStateStub;
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

// TODO: Global for now
static bool g_running;
static win32_offscreen_buffer globalBackBuffer;

static void Win32LoadXInput()
{
  HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
  if(!xInputLibrary)
    {
      xInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
  if(xInputLibrary)
    {
      XInputGetState = (x_input_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
      XInputSetState = (x_input_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
    }
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

static void
Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{
  if(buffer->memory)
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
  buffer->memory = VirtualAlloc(0, bitmapMemorySize,
			      MEM_COMMIT, PAGE_READWRITE);

  buffer->pitch = buffer->width * bytePerPixel;
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

static void
Win32BufferToWindow(HDC deviceContext,
		  win32_offscreen_buffer *buffer,
		  int x, int y, int windowWidth, int windowHeight)
{
  StretchDIBits(deviceContext,
		0, 0, windowWidth, windowHeight,
		0, 0, buffer->width, buffer->height,
	        buffer->memory,
		&buffer->info,
		DIB_RGB_COLORS, SRCCOPY);
}

void Win32InitDSound(HWND window, int32_t bufferSize, int32_t samplePerSecond)
{
  HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");

  if(dSoundLibrary)
    {
      direct_sound_create *DirectSoundCreate = (direct_sound_create *)
	GetProcAddress(dSoundLibrary, "DirectSoundCreate");
      LPDIRECTSOUND directSound;
      if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
	{

	  WAVEFORMATEX waveFormat;
	  waveFormat.wFormatTag = WAVE_FORMAT_PCM; 
	  waveFormat.nChannels = 2; 
	  waveFormat.wBitsPerSample = 16; 
	  waveFormat.nSamplesPerSec = samplePerSecond; 
	  waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8; 
	  waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign; 
	  waveFormat.cbSize = 0;	

	  if(SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
	    {
	      
	      DSBUFFERDESC bufferDescription = {};
	      bufferDescription.dwSize = sizeof(bufferDescription);
	      bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
  
	      
	      LPDIRECTSOUNDBUFFER primaryBuffer;
	      if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
		{	      
		  if(SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
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
	  bufferDescription.dwFlags = 0;
	  bufferDescription.dwBufferBytes = bufferSize;
	  bufferDescription.lpwfxFormat = &waveFormat;
	      
	  LPDIRECTSOUNDBUFFER secondaryBuffer;
	  if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &secondaryBuffer, 0)))
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
  
LRESULT CALLBACK MainWindowCallback(HWND window,
				    UINT message,
				    WPARAM wParam,
				    LPARAM lParam)
{
  LRESULT result = 0;
  
  switch(message)
    {
    case WM_SIZE:
      {
	OutputDebugStringA("WM_SIZE\n");
      } break;
    case WM_DESTROY:
      {
	g_running = false; // TODO: Handle as errro - recreate window?
      } break;
    case WM_ACTIVATEAPP:
      {
	OutputDebugStringA("WM_ACTIVATEAPP\n");
      } break;
    case WM_CLOSE:
      {
	g_running = false; // TODO: Handle as user close message
      } break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
      {
	uint32_t VKCode = wParam;
	bool wasDown = ((lParam & (1 << 30)) != 0);
	bool isDown = ((lParam & (1 << 31)) == 0);
	if(wasDown != isDown)
	  {
	    // TODO: key handle
	  }
	bool32 altKeyWasDown = (lParam & (1 << 29));
	if(VKCode == VK_F4 && altKeyWasDown)
	  {
	    g_running = false;
	  }
      }break;
    case WM_PAINT:
      {
	PAINTSTRUCT paint;
	HDC deviceContext = BeginPaint(window, &paint);
	int x = paint.rcPaint.left;
	int y = paint.rcPaint.top;
	int width = paint.rcPaint.right - paint.rcPaint.left;
	int height = paint.rcPaint.bottom - paint.rcPaint.top;
	Win32BufferToWindow(deviceContext, &globalBackBuffer,
			    0, 0, width, height);
	
	EndPaint(window, &paint);
      } break;
    default:
      {
	result = DefWindowProc(window, message, wParam, lParam);
      } break;
    }
  return result;
}


INT WinMain(HINSTANCE instance,
	    HINSTANCE prevInstance,
	    PSTR cmdLine,
	    INT cmdShow)
{
  WNDCLASSA windowClass = {};
  windowClass.style = CS_HREDRAW|CS_VREDRAW;
  windowClass.lpfnWndProc = MainWindowCallback;
  windowClass.hInstance = instance;
  // windowClass.hIcon = ;
  windowClass.lpszClassName = "MainClass";

  if(RegisterClass(&windowClass))
    {
      HWND window = CreateWindowExA(0, windowClass.lpszClassName,
					 "Main", WS_OVERLAPPEDWINDOW|
					 WS_VISIBLE, CW_USEDEFAULT,
					 CW_USEDEFAULT, CW_USEDEFAULT,
					 CW_USEDEFAULT, 0,
					 0, instance, 0);

      Win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

       if(window)
	{
	  g_running = true;

	  Win32InitDSound(window, 48000, 48000 * sizeof(int16_t) * 2);
	  
	  while(g_running)
	    {
	      MSG message;
	      while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
		  if(message.message == WM_QUIT)
		    {
		      g_running = false;
		    }
		  
		  TranslateMessage(&message);
		  DispatchMessage(&message);
		}

	      for(DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
		{
		  XINPUT_STATE state;
		  if(XInputGetState(i, &state) == ERROR_SUCCESS)
		    {
		      XINPUT_GAMEPAD *pad = &state.Gamepad;

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

	      HDC deviceContext = GetDC(window);
	      win32_window_dimension wd = Win32GetWindowDimension(window);
	      Win32BufferToWindow(deviceContext, &globalBackBuffer,
				  0, 0, wd.width, wd.height);
	      ReleaseDC(window, deviceContext);
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
