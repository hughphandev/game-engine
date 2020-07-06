#include <windows.h>
#include <stdint.h>

// TODO: Global for now
static bool running;
static BITMAPINFO bitmapInfo;
static void *bitmapMemory;
static int bitmapWidth;
static int bitmapHeight;
static int bytePerPixel = 4;

static void
Win32ResizeDIBSection(int width, int height)
{
  if(bitmapMemory)
    {
      VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }

  bitmapWidth = width;
  bitmapHeight = height;
  
  bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth = bitmapWidth;
  bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;

  int bitmapMemorySize = bitmapWidth * bitmapHeight * bytePerPixel;
  bitmapMemory = VirtualAlloc(0, bitmapMemorySize,
			      MEM_COMMIT, PAGE_READWRITE);

  int pitch = bitmapWidth * bytePerPixel;
  uint8_t *row = (uint8_t *)bitmapMemory;
  for(int y = 0; y < bitmapHeight; ++y)
    {
      uint32_t *pixel = (uint32_t *)row;
      for(int x = 0; x < bitmapWidth; ++x)
	{
	  *pixel++ = (255 << 8);
	}
      row += pitch;
    }
}

static void
Win32UpdateWindow(HDC deviceContext,RECT *clientRect,
		  int x, int y, int width, int height)
{
  int windowWidth = clientRect->right - clientRect->left;
  int windowHeight = clientRect->bottom - clientRect->top;
  
  StretchDIBits(deviceContext,
		0, 0, bitmapWidth, bitmapHeight,
		0, 0, windowWidth, windowHeight,
		bitmapMemory,
		&bitmapInfo,
		DIB_RGB_COLORS, SRCCOPY);
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
	RECT clientRect;
	GetClientRect(window, &clientRect);
	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;
	Win32ResizeDIBSection(width, height);
	OutputDebugStringA("WM_SIZE\n");
      } break;
    case WM_DESTROY:
      {
	running = false; // TODO: Handle as errro - recreate window?
      } break;
    case WM_ACTIVATEAPP:
      {
	OutputDebugStringA("WM_ACTIVATEAPP\n");
      } break;
    case WM_CLOSE:
      {
	running = false; // TODO: Handle as user close message
      } break;
    case WM_PAINT:
      {
	PAINTSTRUCT paint;
	HDC deviceContext = BeginPaint(window, &paint);
	int x = paint.rcPaint.left;
	int y = paint.rcPaint.top;
	int width = paint.rcPaint.right - paint.rcPaint.left;
	int height = paint.rcPaint.bottom - paint.rcPaint.top;
	Win32UpdateWindow(deviceContext, &paint.rcPaint,
			  0, 0, width, height);
	
	EndPaint(window, &paint);
      } break;
    default:
      {
	OutputDebugStringA("DEFAULT\n");
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
  WNDCLASS windowClass = {};
  windowClass.style= CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  windowClass.lpfnWndProc = MainWindowCallback;
  windowClass.hInstance = instance;
  // windowClass.hIcon = ;
  windowClass.lpszClassName = "MainClass";

  if(RegisterClass(&windowClass))
    {
      HWND window = CreateWindowEx(0, windowClass.lpszClassName,
					 "Main", WS_OVERLAPPEDWINDOW|
					 WS_VISIBLE, CW_USEDEFAULT,
					 CW_USEDEFAULT, CW_USEDEFAULT,
					 CW_USEDEFAULT, 0,
					 0, instance, 0);
      if(window)
	{
	  running = true;
	  while(running)
	    {
	      MSG message;
	      while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
		  if(message.message == WM_QUIT)
		    {
		      running = false;
		    }
		  
		  TranslateMessage(&message);
		  DispatchMessage(&message);
		}

	      HDC deviceContext = GetDC(window);
	      RECT clientRect;
	      GetClientRect(window, &clientRect);
	      int width = clientRect.right - clientRect.left;
	      int height = clientRect.bottom - clientRect.top;
	      Win32UpdateWindow(deviceContext, &clientRect,
				0, 0, width, height);
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
