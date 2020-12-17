#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdint.h>

#define internal   static
#define global     static
#define persistent static

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32   bool32;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef struct _win32_offscreen_buffer {
    BITMAPINFO Info;
    void *Mem;
    int   Width;
    int   Height;
    int   Pitch;
} win32_offscreen_buffer;

typedef struct _win32_window_dimensions {
    int Width;
    int Height;
} win32_window_dimensions;

// for XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD DwUserIndex, XINPUT_STATE* PState)
typedef X_INPUT_GET_STATE(f_x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global  f_x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// for XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD DwUserIndex, XINPUT_VIBRATION* PVibration)
typedef X_INPUT_SET_STATE(f_x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global  f_x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void Win32LoadXInput(void) {
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!XInputLibrary) {
        XInputLibrary = LoadLibrary("xinput1_3.dll");
        // todo: logging
    }
    if (XInputLibrary) {
        XInputGetState_ = (f_x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState_ = (f_x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else {
        // todo: logging
    }
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) {
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary) {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "");

        // todo: assure this works on windows XP
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
            WAVEFORMATEX WaveFormat = {0};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;
            
            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
                DSBUFFERDESC BufferDescription = {0};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;  // DSBCAPS_GLOBALFOCUS?

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
                        OutputDebugStringA("primary buffer created successfully!\n");
                    }
                    else {
                        // todo: logging    
                    }
                }
                else {
                    // todo: logging    
                }
            }
            else {
                // todo: logging    
            }

            DSBUFFERDESC BufferDescription = {0};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;  // DSBCAPS_GETCURRENTPOSITION?
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            LPDIRECTSOUNDBUFFER SecondaryBuffer;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0))) {
                OutputDebugStringA("secondary buffer created successfully!\n");
            }
            else {
                // todo: logging    
            }
        }
        else {
            // todo: logging
        }
    }
    else {
        // todo: logging
    }
}

global bool32 Running;
global win32_offscreen_buffer GlobalBackBuff;

internal win32_window_dimensions Win32GetWindowDimensions (HWND Window) {
    win32_window_dimensions Result;

    HDC DeviceContext = GetDC(Window);
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right  - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer Buff, int XOff, int YOff) {
    uint8 *Row = (uint8 *)Buff.Mem;
    for (int Y = 0; Y < Buff.Height; ++Y) {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buff.Width; ++X) {
            uint8 Green = (uint8)(X + XOff);
            uint8 Blue  = (uint8)(Y + YOff);

            *Pixel++ = (Green << 8) | Blue;
        }

        Row += Buff.Pitch;
    }
}

internal void Win32DisplayBuffer(
  win32_offscreen_buffer Buff,
  HDC DeviceContext,
  int WindowWidth,
  int WindowHeight
) {

    // todo: aspect ratio correction

    StretchDIBits (
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buff.Width, Buff.Height,
        Buff.Mem,
        &Buff.Info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

internal void Win32ResizeDIBSection (
  win32_offscreen_buffer *Buff,
  int Width,
  int Height
) {

    if (Buff->Mem) {
        VirtualFree(Buff->Mem, 0, MEM_RELEASE);
    }

    Buff->Width = Width;
    Buff->Height = Height;

    Buff->Info.bmiHeader.biSize = sizeof(Buff->Info.bmiHeader);
    Buff->Info.bmiHeader.biWidth = Width;
    Buff->Info.bmiHeader.biHeight = Height;
    Buff->Info.bmiHeader.biPlanes = 1;
    Buff->Info.bmiHeader.biBitCount = 32;
    Buff->Info.bmiHeader.biCompression = BI_RGB;

    int BytesPerPixel = 4;
    int BitmapMemSize = (Width * Height) * BytesPerPixel;
    Buff->Mem = VirtualAlloc(0, BitmapMemSize, MEM_COMMIT, PAGE_READWRITE);
    Buff->Pitch = Width * BytesPerPixel;

    // todo: clear to black
}

internal LRESULT CALLBACK win32MainWindowCallback (
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam
) {
    LRESULT Result = 0;

    switch (Message) {
        case WM_SIZE: {
        } break;
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_CLOSE: {
            Running = false;
        } break;
        case WM_DESTROY: {
            Running = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            uint32 VKCode = (uint32)WParam;
            bool32 WasDown = ((LParam & (1 << 30)) != 0);
            bool32 IsDown  = ((LParam & (1 << 31)) == 0);

            if (VKCode == 'W') {
            }
            else if (VKCode == 'A') {
            }
            else if (VKCode == 'S') {
            }
            else if (VKCode == 'D') {
            }
            else if (VKCode == 'Q') {
            }
            else if (VKCode == 'E') {
            }
            else if (VKCode == VK_UP) {
            }
            else if (VKCode == VK_DOWN) {
            }
            else if (VKCode == VK_LEFT) {
            }
            else if (VKCode == VK_RIGHT) {
            }
            else if (VKCode == VK_ESCAPE) {
            }
            else if (VKCode == VK_SPACE) {
            }

            bool32 AltKeyWasDown = LParam & (1 << 29);
            if ((VKCode == VK_F4) && AltKeyWasDown) {
                Running = false;
            }
        } break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimensions Dimensions = Win32GetWindowDimensions(Window);
            Win32DisplayBuffer (
                GlobalBackBuff,
                DeviceContext,
                Dimensions.Width,
                Dimensions.Height
            );
            EndPaint(Window, &Paint);
        } break;
        default: {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    return Result;
}

int CALLBACK WinMain (
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       CmdShow
) {
    Win32LoadXInput();

    WNDCLASSA WindowClass = {0};
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc   = win32MainWindowCallback;
    WindowClass.hInstance     = Instance;
    WindowClass.lpszClassName = "game";
    
    if (RegisterClassA(&WindowClass)) {
        HWND Window =
            CreateWindowExA (
                0,
                WindowClass.lpszClassName,
                "game",
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

        if (Window) {
            Running = true;

            int XOffset = 0;
            int YOffset = 0;

            Win32InitDSound(Window, 48000, 48000 * sizeof(int16) * 2);

            HDC DeviceContext = GetDC(Window);
            Win32ResizeDIBSection(&GlobalBackBuff, 1920, 1080);

            while (Running) {
                MSG Message;

                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT) {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessage (&Message);
                }

                for (
                  DWORD ControllerIndex = 0;
                  ControllerIndex < XUSER_MAX_COUNT;
                  ControllerIndex++
                ) {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
                        // the controller is plugged in
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool32 Up   = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool32 Left  = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool32 Back  = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool32 LeftShoulder  = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool32 Rightshoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        if (AButton) {
                            YOffset += 2;
                        }
                    }
                    else {
                        // controller unavaliable
                    }
                }
                
                RenderWeirdGradient(GlobalBackBuff, XOffset, YOffset);
                win32_window_dimensions Dimension = Win32GetWindowDimensions(Window);
                Win32DisplayBuffer(GlobalBackBuff, DeviceContext, Dimension.Width, Dimension.Height);

                XOffset++;

            }
        }
        else {  // todo: logging
        }
    }
    else {  // todo: logging
    }

    return 0;
}