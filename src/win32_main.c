#include "game.h"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_main.h"

global bool32 GlobalRunning;
global bool32 GlobalPause;
global win32_offscreen_buffer GlobalBackBuffer;
global LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global int64 GlobalPerfCounterFrequency;

// for XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD DwUserIndex, XINPUT_STATE* PState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// for XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD DwUserIndex, XINPUT_VIBRATION* PVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

DEBUG_PLATFORM_FREE_ENTIRE_FILE(DEBUGPlatformFreeEntireFile)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    DEBUG_read_file_result Res = {0};

    HANDLE FileHandle = CreateFileA (
        Filename,
        GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, 0, NULL
    );
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            Assert(FileSize.QuadPart <= 0xFFFFFFFF);
            Res.Contents = VirtualAlloc (
                0, (size_t)FileSize.QuadPart,
                MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE
            );
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            if (Res.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(FileHandle, Res.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
                {
                    // sucess
                    Res.ContentsSize = FileSize32;
                }
                else
                {
                    //todo
                    //  logging
                    DEBUGPlatformFreeEntireFile(Res.Contents);
                    Res.Contents = 0;
                }
            }
            else
            {
                //todo
                //  logging
            }
        }
        else
        {
            //todo
            //  logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        //todo
        //  logging
    }

    return Res;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool Result = false;
    HANDLE FileHandle = CreateFileA (
        Filename,
        GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, 0, NULL
    );

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if (WriteFile(FileHandle, Memory, (DWORD)Size, &BytesWritten, 0))
        {
            // sucess
            Result = (Size == BytesWritten);
        }
        else
        {
            //TODO
            //  logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
        //TODO
        //  logging
    }

    return Result;
}

typedef struct _win32_game_code
{
    HMODULE GameCodeDLL;
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;

    bool32 IsValid;
} win32_game_code;

internal win32_game_code Win32LoadGameCode(void)
{
    win32_game_code Result = {};
    if (CopyFileA("game.dll", "game_temp.dll", FALSE))
    {
        Result.GameCodeDLL = LoadLibrary("game_temp.dll");
        if (Result.GameCodeDLL)
        {
            Result.UpdateAndRender = (game_update_and_render *)
                GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
    
            Result.GetSoundSamples = (game_get_sound_samples *)
                GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
    
            Result.IsValid = (Result.UpdateAndRender &&
                              Result.GetSoundSamples);
        }
    }

    if (!Result.IsValid)
    {
        Result.UpdateAndRender = GameUpdateAndRenderStub;
        Result.GetSoundSamples = GameGetSoundSamplesStub;
    }

    return Result;
}

void Win32UnloadGameCode(win32_game_code *GameCode)
{
    if (GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRender = GameUpdateAndRenderStub;
    GameCode->GetSoundSamples = GameGetSoundSamplesStub;
}

internal void Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibrary("xinput9_1_0.dll");
        //todo
        //  logging
    }
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibrary("xinput1_3.dll");
        //todo
        //  logging
    }
    if (XInputLibrary)
    {
        XInputGetState_ =
            (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState_ =
            (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else
    {
        //todo
        //  logging
    }
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate =
            (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        //todo
        //  assure this works on windows XP
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound,0)))
        {
            WAVEFORMATEX WaveFormat = {0};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {0};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;  // DSBCAPS_GLOBALFOCUS?

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        OutputDebugStringA("primary buffer created successfully!\n");
                    }
                    else
                    {
                        //todo
                        //  logging
                    }
                }
                else
                {
                    //todo
                    //  logging
                }
            }
            else
            {
                //todo
                //  logging
            }

            DSBUFFERDESC BufferDescription = {0};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;  // DSBCAPS_GETCURRENTPOSITION?
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("secondary buffer created successfully!\n");
            }
            else
            {
                //todo
                //  logging
            }
        }
        else
        {
            //todo
            //  logging
        }
    }
    else
    {
        //todo
        //  logging
    }
}

internal win32_window_dimensions Win32GetWindowDimensions(HWND Window)
{
    win32_window_dimensions Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right  - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void Win32DisplayBuffer (
    win32_offscreen_buffer *Buffer,
    HDC   DeviceContext,
    int32 WindowWidth,
    int32 WindowHeight
)
{
    //todo
    //  aspect ratio correction

    StretchDIBits (
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, SRCCOPY
    );
}

internal void Win32ResizeDIBSection (
    win32_offscreen_buffer *Buffer,
    int32 Width,
    int32 Height
)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
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
    Buffer->BytesPerPixel = BytesPerPixel;

    int32 BitmapMemSize = (Width * Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemSize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * BytesPerPixel;

    //todo
    //  clear to black
}

internal LRESULT CALLBACK win32MainWindowCallback (
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
)
{
    LRESULT Result = 0;

    switch (Message)
    {
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
            Assert(!"NOOOOOOOOOOOOO!!!");
            // keyboad message came from non dispatch message
        } break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimensions Dimensions = Win32GetWindowDimensions(Window);
            Win32DisplayBuffer(&GlobalBackBuffer, DeviceContext, Dimensions.Width, Dimensions.Height);
            EndPaint(Window, &Paint);
        } break;
        default: {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    return Result;
}

internal void Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
      0, SoundOutput->SecondaryBufferSize,
      &Region1, &Region1Size,
      &Region2, &Region2Size,
      0
    )))
    {
        uint8 *DestSample = (uint8 *)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        DestSample = (uint8 *)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void Win32FillSoundBuffer (
    game_sound_buffer  *SourceBuffer,
    win32_sound_output *SoundOutput,
    DWORD ByteToLock,
    DWORD BytesToWrite
)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
      ByteToLock, BytesToWrite,
      &Region1, &Region1Size,
      &Region2, &Region2Size,
      0
    )))
    {
        //todo
        //  assert the sizes of these regions are valid
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        int16 *DestSample = (int16 *)Region1;
        int16 *SourceSample = SourceBuffer->Samples;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        DestSample = (int16 *)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    Assert(NewState->EndedDown != IsDown);
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount;
}

internal void Win32ProcessXInputDigitalButton (
    DWORD XInputButtonState, game_button_state *OldState,
    DWORD ButtonBit, game_button_state *NewState
)
{
    NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal real32 Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    real32 Result = 0;
    if (Value < -DeadZoneThreshold)
    {
        Result = (real32)((Value + DeadZoneThreshold) / (-32768.0f - (real32)DeadZoneThreshold));
    }
    else
    if (Value > DeadZoneThreshold)
    {
        Result = (real32)((Value + DeadZoneThreshold) / (32768.0f - (real32)DeadZoneThreshold));
    }
    return Result;
}

internal void Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
            case WM_QUIT: {
                GlobalRunning = false;
            } break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                uint32 VKCode = (uint32)Message.wParam;
                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown  = ((Message.lParam & (1 << 31)) == 0);
                if (WasDown != IsDown)
                {
                    if (VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    }
                    else if (VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                    else if (VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    }
                    else if (VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    }
                    else if (VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    else if (VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    }
#if BUILD_INTERNAL
                    else if (VKCode == 'P')
                    {
                        if (IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
#endif
                    else if (VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                    }
                    else if (VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                    }
                    else if (VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                    }
                    else if (VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                    }
                    else if (VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
                    else if (VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                    }
                }
                bool32 AltKeyWasDown = Message.lParam & (1 << 29);
                if ((VKCode == VK_F4) && AltKeyWasDown)
                {
                    GlobalRunning = false;
                }
            } break;
            default: {
                TranslateMessage(&Message);
                DispatchMessage (&Message);
            } break;
        }
    }
}

inline LARGE_INTEGER Win32GetWallClockTime(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result =
        (real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCounterFrequency;
    return Result;
}

internal void Win32DrawVertical (
    win32_offscreen_buffer *BackBuffer,
    int32  X,
    int32  Top,
    int32  Bottom,
    uint32 Color
)
{
    if (Top <= 0) 
    {
        Top = 0;
    }
    if (Bottom > BackBuffer->Height)
    {
        Bottom = BackBuffer->Height;
    }

    if ((X >= 0) && (X < BackBuffer->Width))
    {
        uint8 *Pixel = (uint8 *)BackBuffer->Memory + X*BackBuffer->BytesPerPixel + Top*BackBuffer->Pitch;
        for (int32 Y = Top; Y < Bottom; Y++)
        {
            *(uint32 *)Pixel = Color;
            Pixel += BackBuffer->Pitch;
        }   
    }
}

inline void Win32DrawSoundBufferMarker (
    win32_offscreen_buffer *BackBuffer,
    win32_sound_output     *SoundOutput,
    real32 C,
    int32  PadX,
    int32  Top,
    int32  Bottom,
    DWORD  Value,
    uint32 Color
)
{
    real32 Real32X = (C * (real32)Value);
    int32 X = PadX + (int32)Real32X;
    Win32DrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void DEBUGWin32SyncDisplay (
    win32_offscreen_buffer  *BackBuffer,
    int32                    MarkerCount,
    DEBUG_win32_time_marker *Markers,
    int32                    CurrentMarkerIndex,
    win32_sound_output      *SoundOutput,
    real32                   TargetSecondsPerFrame
)
{
    int32 PadX = 16;
    int32 PadY = 16;

    int32 LineHeight = 64;

    real32 C = (real32)(BackBuffer->Width - 2 * PadX) / (real32)SoundOutput->SecondaryBufferSize;
    for (int32 MarkerIndex = 0; MarkerIndex < MarkerCount; MarkerIndex++)
    {
        DEBUG_win32_time_marker *ThisMarker = &Markers[MarkerIndex];
        Assert(ThisMarker->OutputPlayCursor  < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation    < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount   < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor    < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor   < SoundOutput->SecondaryBufferSize);

        DWORD PlayColor = 0x0000FF00;
        DWORD WriteColor = 0xFFFFFFFF;
        DWORD ExpectedFlipColor = 0x000000FF;
        DWORD PlayWindowColor = 0xFFFF00FF;
                                                                                                                                 
        int32 Top = PadY;
        int32 Bottom = Top + LineHeight;
        if (MarkerIndex == CurrentMarkerIndex)
        {
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;
            int32 FirstTop = Top;

            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor,  PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);
            
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation,  PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);
            
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
        }

        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor,  PlayColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + (480 * SoundOutput->BytesPerSample),  PlayColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
    }
}

int CALLBACK WinMain (
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       CmdShow
)
{
    LARGE_INTEGER GlobalPerfCounterFrequencyResult;
    QueryPerformanceFrequency(&GlobalPerfCounterFrequencyResult);
    GlobalPerfCounterFrequency = GlobalPerfCounterFrequencyResult.QuadPart;

    UINT DesiredSchedulerMiliseconds = 1;
    bool32 SleepIsGranuler = (timeBeginPeriod(DesiredSchedulerMiliseconds) == TIMERR_NOERROR);

    Win32LoadXInput();

    WNDCLASSA WindowClass = {0};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1920, 1080);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc   = win32MainWindowCallback;
    WindowClass.hInstance     = Instance;
    WindowClass.lpszClassName = "game";

#define MonitorRefreshFrequency 60
#define GameRefreshFrequency (MonitorRefreshFrequency/2)

    real32 TargetSecondsPerFrame = 1.0f / (real32)GameRefreshFrequency;

    if (RegisterClassA(&WindowClass))
    {
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

        if (Window)
        {
            GlobalRunning = true;

            // sound test
            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.SecondaryBufferSize = SoundOutput.BytesPerSample * SoundOutput.SamplesPerSecond;
            //todo
            //  get riod of FramesOfAudioLatency
            SoundOutput.LatencySampleCount = 3 * (SoundOutput.SamplesPerSecond / GameRefreshFrequency);
            //todo
            //  actuallt compute tis variance and figure out lowest reasonable value is
            SoundOutput.SafetyBytes = ((SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameRefreshFrequency) / 3;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            bool32 SoundIsPlaying = false;

#if BUILD_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes((uint64)1);
#else
            LPVOID BaseAddress = 0;
#endif

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes((uint64)64);
            GameMemory.TransientStorageSize = Gigabytes((uint64)1);
            GameMemory.DEBUGPlatformFreeEntireFile  = DEBUGPlatformFreeEntireFile;
            GameMemory.DEBUGPlatformReadEntireFile  = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            GameMemory.PermanentStorageBytes = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TransientStorageBytes = (uint8 *)GameMemory.PermanentStorageBytes + GameMemory.PermanentStorageSize;

            if (Samples && GameMemory.PermanentStorageBytes)
            {
                HDC DeviceContext = GetDC(Window);

                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClockTime();
                LARGE_INTEGER FlipWallClock = Win32GetWallClockTime();

                int32 DEBUGTimeMarkerIndex = 0;
                DEBUG_win32_time_marker DEBUGTimeMarkers[GameRefreshFrequency / 2] =  {};
                
                DWORD AudioLatencyBytes = 0;
                real32 AudioLatencySeconds = 0.0f;
                bool32 SoundIsValid = false;

                win32_game_code Game = Win32LoadGameCode();
                uint32 LoadCounter = 0;

                int64 LastCycleCount = __rdtsc();
                while (GlobalRunning)
                {
                    if (LoadCounter++ < 120)
                    {
                        Win32UnloadGameCode(&Game);
                        OutputDebugStringA("loading!!\n");
                        Game = Win32LoadGameCode();
                        LoadCounter = 0;
                    }

                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    game_controller_input ZeroController = {};
                    *NewKeyboardController = ZeroController;
                    NewKeyboardController->IsConnected = true;
                    for (int32 ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(NewKeyboardController);

                    if (!GlobalPause)
                    {

                        DWORD MaxControllerCount = XUSER_MAX_COUNT;
                        if (MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                        {
                            MaxControllerCount =  ArrayCount(NewInput->Controllers) - 1;
                        }
                        for (
                          DWORD ControllerIndex = 0;
                          ControllerIndex < MaxControllerCount;
                          ControllerIndex++
                        )
                        {
                            DWORD OurControllerIndex = ControllerIndex + 1;
                            game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                            game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

                            XINPUT_STATE ControllerState;
                            if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                            {
                                // the controller is plugged in
                                NewController->IsConnected = true;

                                XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                                NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                                if ((NewController->StickAverageX != 0.0f) || (NewController->StickAverageX != 0.0f))
                                {
                                    NewController->IsAnalog = true;
                                }

                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                                {
                                    NewController->StickAverageY = 1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                                {
                                    NewController->StickAverageY = -1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                                {
                                    NewController->StickAverageX = -1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                                {
                                    NewController->StickAverageX = 1.0f;
                                    NewController->IsAnalog = false;
                                }

                                real32 Threshold = 0.5f;
                                Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0, &OldController->ActionLeft,  1, &NewController->ActionLeft);
                                Win32ProcessXInputDigitalButton((NewController->StickAverageX >  Threshold) ? 1 : 0, &OldController->ActionRight, 1, &NewController->ActionRight);
                                Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0, &OldController->ActionUp,    1, &NewController->ActionUp);
                                Win32ProcessXInputDigitalButton((NewController->StickAverageY >  Threshold) ? 1 : 0, &OldController->ActionDown,  1, &NewController->ActionDown);

                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown,  XINPUT_GAMEPAD_A, &NewController->ActionDown);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight, XINPUT_GAMEPAD_B, &NewController->ActionRight);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft,  XINPUT_GAMEPAD_X, &NewController->ActionLeft);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp,    XINPUT_GAMEPAD_Y, &NewController->ActionUp);

                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder,  XINPUT_GAMEPAD_LEFT_SHOULDER,  &NewController->LeftShoulder);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start, XINPUT_GAMEPAD_START, &NewController->Start);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back,  XINPUT_GAMEPAD_BACK,  &NewController->Back);
                            }
                            else
                            {
                                // controller unavaliable
                                NewController->IsConnected = false;
                            }
                        }

                        game_video_buffer VideoBuffer = {};
                        VideoBuffer.Memory = GlobalBackBuffer.Memory;
                        VideoBuffer.Width = GlobalBackBuffer.Width;
                        VideoBuffer.Height = GlobalBackBuffer.Height;
                        VideoBuffer.Pitch = GlobalBackBuffer.Pitch;

                        Game.UpdateAndRender(&GameMemory, NewInput, &VideoBuffer);

                        LARGE_INTEGER AudioWallClock = Win32GetWallClockTime();
                        real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                        {
                            /*  
                                note:
                                Here's how the sound output computation works:
                                    A Safety value is defined that is the nunber of
                                samples we think the game update loop may vary by
                                (say up to 2ms).

                                    When the program wakes up to write the audio,
                                it will see what the PlayCursor position is and
                                forecast ahead where it thinks the PlayCursor will
                                be on the next frame boundary.

                                    Then it will look if the WriteCursor is before that
                                by at least the safe amount. If so, the target frame
                                boundary is that frame boundary plus one frame, giving
                                perfect audio sync if the audio card has low enough latency.

                                    If the write cursor is after the safety margin assume
                                that it is impossible to sync the audio perfectly and write
                                one frame's worth of audio plus the safety margin's worth of
                                guard samples.
                            */
                            if (!SoundIsValid)
                            {
                                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                                SoundIsValid = true; 
                            }
                            
                            DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

                            DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameRefreshFrequency;
                            real32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame) * (real32)ExpectedSoundBytesPerFrame);

                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;

                            DWORD SafeWriteCursor = WriteCursor;
                            if (SafeWriteCursor < PlayCursor)
                            {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;
                            bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

                            DWORD TargetCursor = 0;
                            if (AudioCardIsLowLatency)
                            {
                                TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame); 
                            }
                            else
                            {
                                TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
                            }
                            TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

                            DWORD BytesToWrite = 0;
                            if (ByteToLock > TargetCursor)
                            {
                                BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                                BytesToWrite += TargetCursor;
                            }
                            else
                            {
                                BytesToWrite = TargetCursor - ByteToLock;
                            }

                            game_sound_buffer SoundBuffer = {};
                            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;
                            Game.GetSoundSamples(&GameMemory, &SoundBuffer);
#if BUILD_INTERNAL  
                            DEBUG_win32_time_marker *Marker = &DEBUGTimeMarkers[DEBUGTimeMarkerIndex];
                            Marker->OutputPlayCursor = PlayCursor;
                            Marker->OutputWriteCursor = WriteCursor;
                            Marker->OutputLocation = ByteToLock;
                            Marker->OutputByteCount = BytesToWrite;
                            Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                            DWORD UnwrappedWriteCursor = WriteCursor;
                            if (UnwrappedWriteCursor < PlayCursor)
                            {
                                UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                            AudioLatencySeconds = ((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) / (real32)SoundOutput.SamplesPerSecond;

                            char TextBuffer[256];
                            sprintf_s (
                                TextBuffer, sizeof(TextBuffer),
                                "BTL: %u\tTC: %u\tBTW: %uPC: %u\tWC: %u\tDELTA: %u(%fs)\n",
                                ByteToLock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds
                            );
                            OutputDebugStringA(TextBuffer);
#endif
                            Win32FillSoundBuffer(&SoundBuffer, &SoundOutput, ByteToLock, BytesToWrite);
                        }
                        else
                        {
                            SoundIsValid = false;
                        }

                        if (!SoundIsPlaying)
                        {
                            GlobalSecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);
                            SoundIsPlaying = true;
                        }
                        
                        LARGE_INTEGER WorkCounter = Win32GetWallClockTime();
                        real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                    
                        real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                        if (SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            if (SleepIsGranuler)
                            {
                                DWORD SleepMiliseconds = (DWORD)((SecondsElapsedForFrame - TargetSecondsPerFrame));
                                if (SleepMiliseconds > 0)
                                {
                                    Sleep(SleepMiliseconds);
                                }
                            }

                            real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClockTime());

                            if (TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                //todo
                                //  log missed sleep here
                            }

                            while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClockTime());
                            }
                        }
                        else
                        {
                            // missed frame rate!
                            //todo
                            //  logging
                        }

                        LARGE_INTEGER EndCounter = Win32GetWallClockTime();
                        real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                        LastCounter = EndCounter;

                        win32_window_dimensions Dimension = Win32GetWindowDimensions(Window);

#if BUILD_INTERNAL
                        DEBUGWin32SyncDisplay(&GlobalBackBuffer, ArrayCount(DEBUGTimeMarkers), DEBUGTimeMarkers, DEBUGTimeMarkerIndex - 1, &SoundOutput, TargetSecondsPerFrame);
#endif

                        Win32DisplayBuffer(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

                        FlipWallClock = Win32GetWallClockTime();

#if BUILD_INTERNAL
                        {
                            if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                            {
                                Assert(DEBUGTimeMarkerIndex <
                                ArrayCount(DEBUGTimeMarkers));
                                DEBUG_win32_time_marker *Marker = &DEBUGTimeMarkers[DEBUGTimeMarkerIndex];
                                Marker->FlipPlayCursor = PlayCursor;
                                Marker->FlipWriteCursor = WriteCursor;
                            }
                        }
                        
#endif
                        game_input *Temp = NewInput;
                        NewInput = OldInput;
                        OldInput = Temp;

                        uint64 EndCycleCount = __rdtsc();
                        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;  
                        LastCycleCount = EndCycleCount;

                        real32 FramesPerSecond = 0.0f;
                        real32 MCyclesPerFrame = (real32)CyclesElapsed / 1000000.0f;

                        char FPSBuffer[256];
                        sprintf_s(FPSBuffer, sizeof(FPSBuffer), "%f ms/f\t\t%f f/s\t\t%f Mc/f\n", MSPerFrame, FramesPerSecond, MCyclesPerFrame);
                        OutputDebugStringA(FPSBuffer);

#if BUILD_INTERNAL
                        ++DEBUGTimeMarkerIndex;
                        if (DEBUGTimeMarkerIndex == ArrayCount(DEBUGTimeMarkers))
                        {
                            DEBUGTimeMarkerIndex = 0;
                        }
#endif
                    }
                }
            }
            else
            {
                //todo
                //  logging
            }
        }
        else
        {
            //todo
            //  logging
        }
    }
    else
    {
        //todo
        //  logging
    }

    return 0;
}
