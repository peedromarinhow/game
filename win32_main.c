#include <Windows.h>
#include <Xinput.h>
#include <stdint.h>

#define internal   static
#define global     static
#define persistent static

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef struct _win32_offscreen_buffer {
    BITMAPINFO Info;
    void *Mem;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
} win32_offscreen_buffer;

typedef struct _win32_window_dimentions {
    int w;
    int h;
} win32_window_dimentions;

// support for XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD DwUserIndex, XINPUT_STATE* PState)
typedef X_INPUT_GET_STATE(_XInputGetState_);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global _XInputGetState_ *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// support for XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD DwUserIndex, XINPUT_VIBRATION* PVibration)
typedef X_INPUT_SET_STATE(_XInputSetState_);
X_INPUT_SET_STATE(XInputSetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global _XInputSetState_ *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32_LoadXInput(void) {
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!XInputLibrary) {
        XInputLibrary = LoadLibrary("xinput1_3.dll");
    }
    if (XInputLibrary) {
        XInputGetState_ = (_XInputGetState_*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState_ = (_XInputSetState_*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

global bool Running;
global win32_offscreen_buffer GlobalBackBuff;

internal win32_window_dimentions win32GetWindowDimentions (HWND Window) {
    win32_window_dimentions Result;

    HDC DeviceContext = GetDC(Window);
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.w = ClientRect.right  - ClientRect.left;
    Result.h = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer Buff, int XOff, int YOff) {
    uint8 *row = (uint8 *)Buff.Mem;
    for (int y = 0; y < Buff.Height; ++y) {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0; x < Buff.Width; ++x) {
            uint8 green = (uint8)(x + XOff);
            uint8 blue  = (uint8)(y + YOff);

            *pixel++ = (green << 8) | blue;
        }

        row += Buff.Pitch;
    }
}

internal void win32_ResizeDibSection (
  win32_offscreen_buffer *Buff,
  int W,
  int H
) {

    if (Buff->Mem) {
        VirtualFree(Buff->Mem, 0, MEM_RELEASE);
    }

    Buff->Width = W;
    Buff->Height = H;

    Buff->Info.bmiHeader.biSize = sizeof(Buff->Info.bmiHeader);
    Buff->Info.bmiHeader.biWidth = Buff->Width;
    Buff->Info.bmiHeader.biHeight = -(Buff->Height);
    Buff->Info.bmiHeader.biPlanes = 1;
    Buff->Info.bmiHeader.biBitCount = 32;
    Buff->Info.bmiHeader.biCompression = BI_RGB;

    Buff->BytesPerPixel = 4;
    int BitmapMemSize = (Buff->Width * Buff->Width) * Buff->BytesPerPixel;

    Buff->Mem = VirtualAlloc(0, BitmapMemSize, MEM_COMMIT, PAGE_READWRITE);

    Buff->Pitch = W * Buff->BytesPerPixel;

    // todo
    //  clear to black
}

internal void win32_DisplayBuffer(
  win32_offscreen_buffer Buff,
  HDC DeviceContext,
  int WindowWidth,
  int WindowHeight
) {

    // todo
    //  aspect ratio correction

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
            bool wasDown = ((LParam & (1 << 30)) != 0);
            bool isDown  = ((LParam & (1 << 31)) == 0);

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

            bool AltKeyWasDown = (LParam & (1 << 29)) != 0;
            if ((VKCode == VK_F4) && AltKeyWasDown) {
                Running = false;
            }
        } break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int W = Paint.rcPaint.right  - Paint.rcPaint.left;
            int H = Paint.rcPaint.bottom - Paint.rcPaint.top;

            win32_window_dimentions Dimentions = win32GetWindowDimentions(Window);
            win32_DisplayBuffer (
                GlobalBackBuff,
                DeviceContext,
                Dimentions.w,
                Dimentions.h
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
    Win32_LoadXInput();

    WNDCLASSA WindowClass = {};

    win32_ResizeDibSection(&GlobalBackBuff, 1280, 720);

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
            int XOffset = 0;
            int YOffset = 0;

            Running = true;
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

                        bool Up   = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left  = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back  = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftShoulder  = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool Rightshoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        if (AButton) {
                            YOffset += 2;
                        }
                    }
                    else {   // controller unavaliable
                    }
                }
                
                RenderWeirdGradient(GlobalBackBuff, XOffset, YOffset);

                HDC DeviceContext = GetDC(Window);
                win32_window_dimentions Dimentions = win32GetWindowDimentions(Window);
                win32_DisplayBuffer(GlobalBackBuff, DeviceContext, Dimentions.w, Dimentions.h);
                ReleaseDC(Window, DeviceContext);

                XOffset += 2;

            }
        }
        else {   // TODO
        }
    }
    else {   // TODO
    }

    return 0;
}
