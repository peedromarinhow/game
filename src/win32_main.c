#define internal         static
#define global           static
#define local_persistent static

#include <stdint.h>

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32   bool32;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float  real32;
typedef double real64;

#define PI32 3.14159265359f

#include <windows.h>
#include <xinput.h>
#include <dsound.h>

#include <stdio.h>
#include <math.h>
#include <malloc.h>

#include "game.cpp"
#include "win32_main.h"

global bool32 GlobalRunning;
global win32_offscreen_buffer GlobalBackBuffer;
global LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

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
        XInputLibrary = LoadLibrary("xinput9_1_0.dll");
        // todo: logging
    }
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
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        // todo: assure this works on windows XP
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound,0))) {
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
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0))) {
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

internal win32_window_dimensions Win32GetWindowDimensions (HWND Window) {
    win32_window_dimensions Result;

    HDC DeviceContext = GetDC(Window);
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right  - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int32 BlueOffset, int32 GreenOffset)
{
    uint8_t *Row = (uint8_t *)Buffer.Mem;
    for (int32 Y = 0;Y < Buffer.Height; Y++) {
        uint32_t *Pixel = (uint32_t *)Row;
        for (int32 X = 0; X < Buffer.Width; X++) {
            // memory Order: BB GG RR XX
            // 0xXXRRGGBB
            uint8_t Blue = X + BlueOffset;
            uint8_t Green = Y + GreenOffset;
            *Pixel++ = ((Green << 8) | Blue );
        }
        Row += Buffer.Pitch;
    }
}

internal void Win32DisplayBuffer(
  win32_offscreen_buffer Buffer,
  HDC DeviceContext,
  int32 WindowWidth,
  int32 WindowHeight
) {
    // todo: aspect ratio correction

    StretchDIBits (
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Mem,
        &Buffer.Info,
        DIB_RGB_COLORS, SRCCOPY
    );
}

internal void Win32ResizeDIBSection (
  win32_offscreen_buffer *Buffer,
  int32 Width,
  int32 Height
) {
    if (Buffer->Mem) {
        VirtualFree(Buffer->Mem, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Width;
    Buffer->Info.bmiHeader.biHeight = Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int32 BytesPerPixel = 4;
    int32 BitmapMemSize = (Width * Height) * BytesPerPixel;
    Buffer->Mem = VirtualAlloc(0, BitmapMemSize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * BytesPerPixel;

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
            GlobalRunning = false;
        } break;
        case WM_DESTROY: {
            GlobalRunning = false;
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
                GlobalRunning = false;
            }
        } break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimensions Dimensions = Win32GetWindowDimensions(Window);
            Win32DisplayBuffer(GlobalBackBuffer, DeviceContext, Dimensions.Width, Dimensions.Height);
            EndPaint(Window, &Paint);
        } break;
        default: {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    return Result;
}

internal void Win32ClearSoundBuffer(win32_sound_output *SoundOutput) {
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
      0, SoundOutput->SecondaryBufferSize,
      &Region1, &Region1Size,
      &Region2, &Region2Size,
      0
    ))) {
        uint8 *DestSample = (uint8 *)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex) {
            *DestSample++ = 0;
        }
        DestSample = (uint8 *)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex) {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void Win32FillSoundBuffer (game_sound_buffer *SourceBuffer, win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite) {
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
      ByteToLock, BytesToWrite,
      &Region1, &Region1Size,
      &Region2, &Region2Size,
      0
    ))) {
        // todo: assert the sizes of these regions are valid
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        int16 *DestSample = (int16 *)Region1;
        int16 *SourceSample = SourceBuffer->Samples;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        DestSample = (int16 *)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, game_button_state *NewState) {
    NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

int CALLBACK WinMain (
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       CmdShow
) {
    LARGE_INTEGER PerfCounterFrequencyResult;
    QueryPerformanceFrequency(&PerfCounterFrequencyResult);
    int64 PerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;

    Win32LoadXInput();

    WNDCLASSA WindowClass = {0};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1920, 1080);
    
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
            GlobalRunning = true;

            // sound test
            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.SecondaryBufferSize = SoundOutput.BytesPerSample * SoundOutput.SamplesPerSecond;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_COMMIT, PAGE_READWRITE);
            bool32 SoundIsPlaying = false;

#if BUILD_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(1);
#else
            LPVOID BaseAddress = 0;
#endif

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes((uint64)1);
            uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            GameMemory.PermanentStorageBytes = VirtualAlloc(BaseAddress, TotalSize, MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TransientStorageBytes = (uint8 *)GameMemory.PermanentStorageBytes + GameMemory.PermanentStorageSize;

            if (Samples && GameMemory.PermanentStorageBytes) {
                HDC DeviceContext = GetDC(Window);

                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter;
                QueryPerformanceCounter(&LastCounter);
                int64 LastCycleCount = __rdtsc();
                while (GlobalRunning) {
                    MSG Message;

                    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                        if (Message.message == WM_QUIT) {
                            GlobalRunning = false;
                        }

                        TranslateMessage(&Message);
                        DispatchMessage (&Message);
                    }

                    int32 MaxControllerCount = XUSER_MAX_COUNT;
                    if (MaxControllerCount > ArrayCount(NewInput->Controllers)) {
                        MaxControllerCount = ArrayCount(NewInput->Controllers);
                    }
                    for (
                      DWORD ControllerIndex = 0;
                      ControllerIndex < MaxControllerCount;
                      ControllerIndex++
                    ) {
                        game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
                        game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];

                        XINPUT_STATE ControllerState;
                        if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
                            // the controller is plugged in
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                            bool32 Up   = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                            bool32 Down = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                            bool32 Left  = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                            bool32 Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

                            NewController->StartX = OldController->EndX;
                            NewController->StartY = OldController->EndY;

                            real32 X;
                            if (Pad->sThumbLX < 0) {
                                X = (real32)Pad->sThumbLX / -32768.0f;
                            }
                            else {
                                X = (real32)Pad->sThumbLX / 32768.0f;
                            }
                            NewController->MinX = NewController->MaxX = NewController->EndX = X;

                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Down, XINPUT_GAMEPAD_A, &NewController->Down);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Right, XINPUT_GAMEPAD_B, &NewController->Right);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Left, XINPUT_GAMEPAD_X, &NewController->Left);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Up, XINPUT_GAMEPAD_Y, &NewController->Up);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

                            // bool32 Start = Pad->wButtons & XINPUT_GAMEPAD_START;
                            // bool32 Back  = Pad->wButtons & XINPUT_GAMEPAD_BACK;
                        }
                        else {
                            // controller unavaliable
                        }
                    }

                    DWORD ByteToLock;
                    DWORD BytesToWrite;
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    bool32 SoundIsValid = false;
                    if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
                        SoundIsValid = true;
                        ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                        DWORD TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;
                        if (ByteToLock > TargetCursor) {
                            BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                            BytesToWrite += TargetCursor;
                        }
                        else {
                            BytesToWrite = TargetCursor - ByteToLock;
                        }
                    }

                    game_sound_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                    SoundBuffer.Samples = Samples;

                    game_video_buffer VideoBuffer = {};
                    VideoBuffer.Mem = GlobalBackBuffer.Mem;
                    VideoBuffer.Width = GlobalBackBuffer.Width;
                    VideoBuffer.Height = GlobalBackBuffer.Height;
                    VideoBuffer.Pitch = GlobalBackBuffer.Pitch;

                    GameUpdateAndRender(&GameMemory, NewInput, &SoundBuffer, &VideoBuffer);

                    // directsound test
                    //      note: did not understand a thing about how this works
                    if (SoundIsValid) {
                        Win32FillSoundBuffer(&SoundBuffer, &SoundOutput, ByteToLock, BytesToWrite);
                    }

                    if (!SoundIsPlaying) {
                        GlobalSecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);
                        SoundIsPlaying = true;
                    }
                    win32_window_dimensions Dimension = Win32GetWindowDimensions(Window);
                    Win32DisplayBuffer(GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

                    int64 EndCycleCount = __rdtsc();
                    LARGE_INTEGER EndCounter;
                    QueryPerformanceCounter(&EndCounter);

                    // todo: print this counter here
                    int64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    real32 MCyclesPerFrame = (real32)CyclesElapsed / 1000000.0f;
                    int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                    real32 MSPerFrame = (1000.0f * (real32)CounterElapsed) / (real32)PerfCounterFrequency;
                    real32 FramesPerSecond = (real32)PerfCounterFrequency / (real32)CounterElapsed;

                    char Buffer[256];
                    sprintf(Buffer, "%f ms/f\t\t%f f/s\t\t%f Mc/f\n", MSPerFrame, FramesPerSecond, MCyclesPerFrame);
                    OutputDebugStringA(Buffer);

                    LastCounter = EndCounter;
                    LastCycleCount = EndCycleCount;

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;
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
    else {
        // todo: logging
    }

    return 0;
}
