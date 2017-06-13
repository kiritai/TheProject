/*
	ACE Engine
*/

#include <Windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>

#define local_persist static 
#define global_variable static 
#define internal static 

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer
{
	BITMAPINFO	Info;
	void		*Memory;
	int			Width;
	int			Height;
	int			Pitch;
};

struct win32_window_dimensions
{
	int Width;
	int Height;
};

// TODO(Howt3ch) : This is a global for now
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32LoadXInput(void)
{
	// TODO(Howt3ch) : Test this on windows 8
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		// TODO(Howt3ch) : Diagnostic
		HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState) { XInputGetState = XInputGetStateStub; }

		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState) { XInputSetState = XInputSetStateStub; }

		// TODO(Howt3ch) : Diagnostic
	}
	else
	{
		// TODO(Howt3ch) : Diagnostic
	}
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	// NOTE(Howt3ch) : Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if (DSoundLibrary)
	{
		// NOTE(Howt3ch) : Get a DirectSound object
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		// TODO(Howt3ch) : Double-check that this works on XP
		LPDIRECTSOUND8 DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				// NOTE(Howt3ch) : Create a primary buffer
				// TODO(Howt3ch) : DSBCAPS_GLOBALFOCUS?
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0);
				if (SUCCEEDED(Error))
				{
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						// NOTE(Howt3ch) : We have finally set the format
					}
					else
					{
						// TODO(Howt3ch) : Diagnostic
					}
				}
				else
				{
					// TODO(Howt3ch) : Diagnostic
				}
			}
			else
			{
				// TODO(Howt3ch) : Diagnostic
			}

			// NOTE(Howt3ch) : DSBCAPS_GETCURRENTPOSITION2
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{

			}
		}
		else
		{
			// TODO(Howt3ch) : Diagnostic
		}
	}
	else
	{
		// TODO(Howt3ch) : Diagnostic
	}
}

internal win32_window_dimensions Win32GetWindowDimension(HWND Window)
{
	win32_window_dimensions Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return (Result);
}

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
	// TODO(Howt3ch) : Let's see what the optimizer does
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			/*
							 xx RR GG BB
			Pixel in memory: 00 00 00 00
		 
			LITTLE ENDIAN ARCHITECTURE
			*/

			uint8 Blue = (X + XOffset);
			uint8 Green = (Y + YOffset);

			*Pixel++ = (Green << 8 | Blue);
		}

		Row += Buffer->Pitch;
	}
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	// TODO(Howt3ch) : Bulletproof this
	// > Maybe don't free first, free after, then free first if that fails

	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	int BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize			= sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth			= Buffer->Width;
	Buffer->Info.bmiHeader.biHeight			= -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes			= 1;
	Buffer->Info.bmiHeader.biBitCount		= 32;
	Buffer->Info.bmiHeader.biCompression	= BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Width * BytesPerPixel;

	// TODO(Howt3ch) : Probably clear this to black
}

internal void Win32DisplayBufferInWindow(
		HDC DeviceContext, 
		int WindowWidth, int WindowHeight,
		win32_offscreen_buffer *Buffer,
		int X, int Y, 
		int Width, int Height
	)
{
	// TODO(Howt3ch) : Aspect ratio correction
	StretchDIBits(
			DeviceContext,
			/*
			X, Y, Width, Height,
			X, Y, Width, Height,
			*/
			0, 0, WindowWidth, WindowHeight,
			0, 0, Buffer->Width, Buffer->Height,
			Buffer->Memory,
			&Buffer->Info,
			DIB_RGB_COLORS, SRCCOPY
		);
}

LRESULT CALLBACK Win32MainWindowCallback(
		HWND	Window,
		UINT	Message,
		WPARAM	wParam,
		LPARAM	lParam
	)
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_SIZE:
		{
		} break;

		case WM_DESTROY:
		{
			// TODO(Howt3ch) : Handle this as an error - recreate window?
			GlobalRunning = false;
		} break;

		case WM_CLOSE:
		{
			// TODO(Howt3ch) : Handle this with a message to the user?
			GlobalRunning = false;
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_SYSKEYDOWN:
		{

		} break;

		case WM_SYSKEYUP:
		{

		} break;

		case WM_KEYDOWN:
		{

		} break;

		case WM_KEYUP:
		{
			uint32 VKCode = wParam;
			bool WasDown = (lParam & (1 << 30) != 0);
			bool IsDown = (lParam & (1 << 31) == 0);

			if (WasDown != IsDown)
			{
				if (VKCode == 'W')
				{

				}
				else if (VKCode == 'A')
				{

				}
				else if (VKCode == 'S')
				{

				}
				else if (VKCode == 'D')
				{

				}
				else if (VKCode == 'Q')
				{

				}
				else if (VKCode == 'Q')
				{

				}
				else if (VKCode == VK_UP)
				{

				}
				else if (VKCode == VK_DOWN)
				{

				}
				else if (VKCode == VK_LEFT)
				{

				}
				else if (VKCode == VK_RIGHT)
				{

				}
				else if (VKCode == VK_ESCAPE)
				{

				}
				else if (VKCode == VK_SPACE)
				{

				}
			}

			bool32 AltKeyWasDown = (lParam & (1 << 29));
			if ((VKCode == VK_F4) && AltKeyWasDown)
			{
				GlobalRunning = false;
			}
		} break;

		case WM_PAINT: 
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext		= BeginPaint(Window, &Paint);
			int X					= Paint.rcPaint.left;
			int Y					= Paint.rcPaint.top;
			int Width				= Paint.rcPaint.right - Paint.rcPaint.left;
			int Height				= Paint.rcPaint.bottom - Paint.rcPaint.top;

			win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackbuffer, X, Y, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);
		} break;

		default:
		{
			//OutputDebugStringA("default\n");
			Result = DefWindowProc(Window, Message, wParam, lParam);
		} break;
	}

	return(Result);
}

// > Entry point for the OS
int CALLBACK WinMain(
		HINSTANCE	Instance,
		HINSTANCE	PrevInstance,
		LPSTR		CommandLine,
		int			ShowCode
	)
{
	Win32LoadXInput();

	WNDCLASS WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.style			= CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc		= Win32MainWindowCallback;
	WindowClass.hInstance		= Instance;
	//WindowClass.hIcon;
	WindowClass.lpszClassName	= "TheProjectWindowClass";

	if (RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowEx(
				0,
				WindowClass.lpszClassName,
				"The Project",
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				0,
				0,
				Instance,
				0
			);

		if (Window)
		{
			HDC DeviceContext = GetDC(Window);

			int XOffset = 0;
			int	YOffset = 0;

			int ToneHz = 256;
			uint32 RunningSampleIndex = 0;
			int SamplesPerSecond = 48000;
			int SquareWavePeriod = SamplesPerSecond / ToneHz;
			int HalfSquareWavePeriod = SquareWavePeriod / 2;
			int BytesPerSample = sizeof(int16) * 2;
			int SecondaryBufferSize = SamplesPerSecond * BytesPerSample;
			int ToneVolume = 8000;

			Win32InitDSound(Window, SamplesPerSecond, SecondaryBufferSize);
			bool32 soundIsPlaying = false;

			GlobalRunning = true;
			while (GlobalRunning)
			{
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				// TODO(Howt3ch) : Should we poll this more frequently?
				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// NOTE(Howt3ch) : This controller is plugged in
						// TODO(Howt3ch) : See if ControllerState.dwPacketNumber increments too quick
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

						bool Up				= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down			= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left			= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right			= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start			= (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back			= (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder	= (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder	= (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton		= (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton		= (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton		= (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton		= (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 StickX		= Pad->sThumbLX;
						int16 StickY		= Pad->sThumbLY;

						if (AButton)
						{
							++YOffset;
						}
					}
					else
					{
						// NOTE(Howt3ch) : This controller is not available
					}
				}

				RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);
				
				// NOTE(Howt3ch) : DirectSound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					DWORD ByteToLock = RunningSampleIndex * BytesPerSample % SecondaryBufferSize;
					DWORD BytesToWrite;
					if (ByteToLock == PlayCursor)
					{
						BytesToWrite = SecondaryBufferSize;
					}
					else if (BytesPerSample > PlayCursor)
					{
						BytesToWrite = (SecondaryBufferSize - ByteToLock);
						BytesToWrite += PlayCursor;
					} 
					else
					{
						BytesToWrite = PlayCursor - ByteToLock;
					}
					
					VOID *Region1;
					DWORD Region1Size;
					VOID *Region2;
					DWORD Region2Size;

					if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
							&Region1, &Region1Size,
							&Region2, &Region2Size,
							0
						)))
					{
						// TODO(Howt3ch) : assert that region1/2size is valid
						DWORD Region1SampleCount = Region1Size / BytesPerSample;
						int16 *SampleOut = (int16 *)Region1;
						for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
						{
							int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}

						DWORD Region2SampleCount = Region2Size / BytesPerSample;
						SampleOut = (int16 *)Region2;
						for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
						{
							int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}

						GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
					}
				}

				if (!soundIsPlaying)
				{
					GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
					soundIsPlaying = true;
				}

				win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, 
					&GlobalBackbuffer, 0, 0, Dimension.Width, Dimension.Height);
			}

			ReleaseDC(Window, DeviceContext);
		}
		else
		{
			// TODO(Howt3ch) : Logging
		}
	}
	else
	{
		// TODO(Howt3ch) : Logging
	}

	return(0);
}