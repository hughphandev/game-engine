#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H


typedef int32_t bool32;

#define X_INPUT_SET_STATE(name) \
  DWORD name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibrationdt)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }

#define X_INPUT_GET_STATE(name) \
  DWORD name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
#define DIRECT_SOUND_CREATE(name)                                \
  HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, \
                      LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

static x_input_set_state* XInputSetState_ = XInputSetStateStub;
static x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

struct win32_window_dimension
{
	int width;
	int height;
};

struct win32_offscreen_buffer
{
	BITMAPINFO info;
	void* memory;
	int width;
	int height;

	// relative to window
	v2 offSet;
};

// NOTE: considering adding bytesPerSample to simplify calculation;
struct win32_sound_output
{
	uint32_t runningSampleIndex;
	DWORD bytesPerSample;
	DWORD samplesPerSecond;
	DWORD secondaryBufferSize;
	DWORD safetyBytes;
};

struct win32_game_code
{
	HMODULE gameDll;
	game_update_and_render* UpdateAndRender;
	game_output_sound* OutputSound;
	FILETIME lastWriteTime;

	bool isValid;
};

#if INTERNAL
struct debug_audio_marker
{
	DWORD* playCursors;
	int playCursorCount;
	DWORD writeCursor;

	DWORD nextFrameBoundary;
	DWORD flipPlayCursor;
	DWORD flipWriteCursor;

	DWORD expectedWritePosition;
	DWORD expectedTargetCursor;
};

struct win32_state
{
	char* fileFullPath;

	size_t recordIndex;
	HANDLE playHandle;

	size_t playIndex;
	HANDLE recordHandle;

	void* gameMemory;
	DWORD memorySize;
	//TODO: remove if write to disk is viable!
	void* tempGameMemory;

	bool isRecording;
	bool isPlaying;
};
#endif

#endif