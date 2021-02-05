/*
 *  FOR THE WHOLE PLATFORM:
 *
 *  observations:
 *      I   - for some reason it draws the video buffer upside-down,
 *            solving by setting Buffer->Info.bmiHeader.biHeight = -Height
 *      II  - the sine wave output frequency seems to be one octave
 *            down from casey's (half the frequency)
 *      III - casey dumps the whole win32_state::AppMemoryBlock to ram
 *            instead of to disk in https://youtu.be/es-Bou2dIdY?t=2000
 *            this computer cannot handle that, so skipping this
 *
 *  todo:
 *      // II  - process mouse messages together with the rest of the input
 *      III - maybe separate all these functions to different files
 *
 *
 *  this whole thing seems wicked
 */


#include "app.h"

#include <dsound.h>
#include <malloc.h>
#include <stdio.h>
#include <windows.h>
#include <xinput.h>
#include <gl/gl.h>

#include "win32_main.h"

global b32 GlobalRunning;
global b32 GlobalPause;
global LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global i64 GlobalPerfCounterFrequency;
global b32 GlobalShowCursor;
global WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };

internal void Win23ToggleFullScreen(HWND Window)
{
    // note
    //  copied from https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    //  by Raymond Chen
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}



// string stuff
void CatStrings(size_t SourceACount, char *SourceA,
                size_t SourceBCount, char *SourceB,
                size_t DestCount, char *Dest)
{
    for (size_t Index = 0; Index < SourceACount; Index++)
    {
        *Dest++ = *SourceA++;
    }
    for (size_t Index = 0; Index < SourceBCount; Index++)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = '\0';
}

internal void Win32GetEXEFilename(win32_state *State)
{
    DWORD SizeofFileName =
      GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for (char *Scan = State->EXEFileName; *Scan; ++Scan)
    {
        if (*Scan == '\\')
            State->OnePastLastEXEFileNameSlash = Scan + 1;
    }
}

internal int StringLenght(char *String)
{
    int Count = 0;
    while (*String++)
    {
        Count++;
    }
    return Count;
}

internal void Win32BuildEXEPathFilename(win32_state *State, char *Filename,
                                        i32 DestCount, char *Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName,
               State->EXEFileName, StringLenght(Filename), Filename,
               DestCount, Dest);
}



// input stuff
// for XInputGetState
#define X_INPUT_GET_STATE(name) \
    DWORD WINAPI name(DWORD DwUserIndex, XINPUT_STATE *PState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// for XInputSetState
#define X_INPUT_SET_STATE(name) \
    DWORD WINAPI name(DWORD DwUserIndex, XINPUT_VIBRATION *PVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_



#define DIRECT_SOUND_CREATE(name) \
    HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);



// file stuff
DEBUG_PLATFORM_FREE_ENTIRE_FILE(DEBUGPlatformFreeEntireFile)
{
    if (Memory)
        VirtualFree(Memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {0};

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ,
                                    FILE_SHARE_READ, NULL,
                                    OPEN_EXISTING, 0, NULL);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            Assert(FileSize.QuadPart <= 0xFFFFFFFF);
            Result.Contents =
                VirtualAlloc(0, (size_t)FileSize.QuadPart,
                             MEM_RESERVE | MEM_COMMIT,
                             PAGE_READWRITE);
            u32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            if (Result.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                    (FileSize32 == BytesRead))
                {
                    // sucess
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    // todo
                    //  logging
                    DEBUGPlatformFreeEntireFile(Thread, Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {
                // todo
                //  logging
            }
        }
        else
        {
            // todo
            //  logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // todo
        //  logging
    }

    return Result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool Result = false;
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE,
                                    0, NULL,
                                    CREATE_ALWAYS, 0, NULL);

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
            // TODO
            //  logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
        // TODO
        //  logging
    }

    return Result;
}

inline FILETIME GetLastFileWriteTime(char *Filename)
{
    FILETIME Result = {};

#if 0
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle = FindFirstFileA(FileName, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE)
    {
        Result = FindData.ftLastWriteTime;
        FindClose(FindHandle);
    }
#else
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
        Result = Data.ftLastWriteTime;
#endif

    return Result;
}



// code loading stuff
internal win32_app_code Win32LoadAppCode(char *SourceDLLName, char *TempDLLName)
{
    win32_app_code Result = {};

    Result.DLLLastWriteTime = GetLastFileWriteTime(SourceDLLName);
    while (true)
    {
        if (CopyFileA(SourceDLLName, TempDLLName, FALSE))
            break;
    }
    Result.AppCodeDLL = LoadLibrary(TempDLLName);
    if (Result.AppCodeDLL)
    {
        Result.UpdateAndRender =
            (app_update_and_render *)GetProcAddress(Result.AppCodeDLL, "AppUpdateAndRender");

        Result.GetSoundSamples =
            (app_get_sound_samples *)GetProcAddress(Result.AppCodeDLL, "AppGetSoundSamples");

        Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
    }

    if (!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
        Result.GetSoundSamples = 0;
    }

    return Result;
}

void Win32UnloadAppCode(win32_app_code *AppCode)
{
    if (AppCode->AppCodeDLL)
    {
        FreeLibrary(AppCode->AppCodeDLL);
        AppCode->AppCodeDLL = 0;
    }

    AppCode->IsValid = false;
    AppCode->UpdateAndRender = 0;
    AppCode->GetSoundSamples = 0;
}

internal void Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibrary("xinput9_1_0.dll");
        // todo
        //  logging
    }
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibrary("xinput1_3.dll");
        // todo
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
        // todo
        //  logging
    }
}

internal void Win32InitDSound(HWND Window, i32 SamplesPerSecond, i32 BufferSize)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate =
            (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        // todo
        //  assure this works on windows XP
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
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
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER; // DSBCAPS_GLOBALFOCUS?

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        OutputDebugStringA("primary buffer created successfully!\n");
                        //
                    }
                    else
                    {
                        // todo
                        //  logging
                    }
                }
                else
                {
                    // todo
                    //  logging
                }
            }
            else
            {
                // todo
                //  logging
            }

            DSBUFFERDESC BufferDescription = {0};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2; // DSBCAPS_GETCURRENTPOSITION?
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("secondary buffer created successfully!\n");
                //
            }
            else
            {
                // todo
                //  logging
            }
        }
        else
        {
            // todo
            //  logging
        }
    }
    else
    {
        // todo
        //  logging
    }
}



// sound stuff
internal void Win32FillSoundBuffer(app_sound_buffer *SourceBuffer, win32_sound_output *SoundOutput,
                                   DWORD ByteToLock, DWORD BytesToWrite)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                             &Region1, &Region1Size,
                                             &Region2, &Region2Size, 0)))
    {
        // todo
        //  assert the sizes of these regions are valid
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        i16 *DestSample = (i16 *)Region1;
        i16 *SourceSample = SourceBuffer->Samples;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        DestSample = (i16 *)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                             &Region1, &Region1Size,
                                             &Region2, &Region2Size, 0)))
    {
        u8 *DestSample = (u8 *)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        DestSample = (u8 *)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size,
                                      Region2, Region2Size);
    }
}



// opengl stuff?
internal void Win32InitOpenGl(HWND Window)
{
    HDC WindowDC = GetDC(Window);

    // todo wtf
    //  cColorBits supposed to exclude alpha bits?
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
    DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
    DesiredPixelFormat.cColorBits = 32;
    DesiredPixelFormat.cAlphaBits = 8;
    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

    i32 SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);

    HGLRC OpenGLRC = wglCreateContext(WindowDC);
    if (wglMakeCurrent(WindowDC, OpenGLRC))
    {
        // note
        // sucess!
    }
    else
    {
        Assert(!"NOOOOOOOOOOOO!!");
        // invalid code path
    }
    ReleaseDC(Window, WindowDC);
}



// windows stuff
internal win32_window_dimensions Win32GetWindowDimensions(HWND Window)
{
    win32_window_dimensions Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void Win32DisplayBuffer(HDC DeviceContext,
                                 i32 WindowWidth, i32 WindowHeight)
{
#if 0
    i32 OffsetX = 10;
    i32 OffsetY = 10;

    PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
    PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

    StretchDIBits(DeviceContext, OffsetX, OffsetY,
                  Buffer->Width, Buffer->Height, 0, 0,
                  Buffer->Width, Buffer->Height,
                  Buffer->Memory, &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
#endif
    glViewport(0, 0, WindowWidth, WindowHeight);

    // todo
    //  remove this
    GLuint TextureHandle = 0;
    static b32 Init = false;
    if (!Init)
    {
        glGenTextures(1, &TextureHandle);
        Init = true;
    }

    glBindTexture(GL_TEXTURE_2D, TextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Buffer->Width, Buffer->Height, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Buffer->Memory);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_TEXTURE_2D);

    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glScalef(1.0f, -1.0f, 1.0f);
    // todo
    //  for now scaling by -1 along y because the GlobalBackBuffer is being displayed upside
    //  down for some reason, wich I couldn't find
    // note
    //  stupidity

    glBegin(GL_TRIANGLES);

    r32 P = 1.0f;

    // lower tri
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-P,-P);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(P, -P);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(P, P);

    // higher tri
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-P, -P);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(P,P);

    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-P, P);

    glEnd();

    SwapBuffers(DeviceContext);
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer,
                                    i32 Width, i32 Height)
{
    if (Buffer->Memory)
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);

    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Width;
    Buffer->Info.bmiHeader.biHeight = Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    i32 BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;

    i32 BitmapMemSize = (Width * Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemSize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * BytesPerPixel;

    // todo
    //  clear to black
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                                  WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_ACTIVATEAPP:
        {
#if 0
            if (WParam == TRUE)
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
            }
            else
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 64, LWA_ALPHA);
            }
#endif
        }
        break;
        case WM_CLOSE:
        {
            GlobalRunning = false;
        }
        break;
        case WM_DESTROY:
        {
            GlobalRunning = false;
        }
        break;

        case WM_SETCURSOR:
        {
            if (GlobalShowCursor)
            {
                Result = DefWindowProcA(Window, Message, WParam, LParam);
            }
            else
            {
                SetCursor(0);
            }
        }
        break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"NOOOOOOOOOOOOO!!!");
            // keyboad message came from non dispatch message
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimensions Dimensions = Win32GetWindowDimensions(Window);
            Win32DisplayBuffer(DeviceContext, Dimensions.Width, Dimensions.Height);
            EndPaint(Window, &Paint);
        }
        break;
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        }
        break;
    }
    return Result;
}

internal void Win32ProcessKeyboardMessage(app_button_state *NewState, b32 IsDown)
{
    if (NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}



// other (?) input stuff
internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, app_button_state *OldState,
                                              DWORD ButtonBit, app_button_state *NewState)
{
    NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal r32 Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    r32 Result = 0;
    if (Value < -DeadZoneThreshold)
    {
        Result = (r32)((Value + DeadZoneThreshold) /
            (-32768.0f - (r32)DeadZoneThreshold));
    }
    else if (Value > DeadZoneThreshold)
    {
        Result = (r32)((Value + DeadZoneThreshold) /
            (32768.0f - (r32)DeadZoneThreshold));
    }
    return Result;
}

internal void Win32GetInputFileLocation(win32_state *State,
                                        b32 InputStream, i32 SlotIndex,
                                        i32 DestCount, char *Dest)
{
    char Temp[64];
    wsprintf(Temp, "looped_edit_%d_%s.gmi", SlotIndex, InputStream ? "input" : "state");
    Win32BuildEXEPathFilename(State, Temp, DestCount, Dest);
}

internal win32_replay_buffer *Win32GetReplayBuffer(win32_state *State, int unsigned Index)
{
    Assert(Index < ArrayCount(State->ReplayBuffers));
    return &State->ReplayBuffers[Index];
}

internal void Win32BeginRecordingInput(win32_state *State, i32 InputRecordingIndex)
{
    win32_replay_buffer *Buffer = Win32GetReplayBuffer(State, InputRecordingIndex);
    if (Buffer->MemoryBlock)
    {
        State->InputRecordingIndex = InputRecordingIndex;
        char Filename[WIN32_STATE_FILENAME_COUNT];
        Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(Filename), Filename);
        State->RecordingHandle = CreateFileA(Filename, GENERIC_WRITE,
                                             0, NULL,
                                             CREATE_ALWAYS, 0, NULL);

#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = State->TotalSize;
        SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif

        CopyMemory(Buffer->MemoryBlock, State->AppMemoryBlock, (size_t)State->TotalSize);
    }
}

internal void Win32EndRecordingInput(win32_state *State)
{
    CloseHandle(State->RecordingHandle);
    State->InputRecordingIndex = 0;
}

internal void Win32BeginInputPlayback(win32_state *State, i32 InputPlaybackIndex)
{
    win32_replay_buffer *Buffer = Win32GetReplayBuffer(State, InputPlaybackIndex);
    if (Buffer->MemoryBlock)
    {
        State->InputPlaybackIndex = InputPlaybackIndex;
        char Filename[WIN32_STATE_FILENAME_COUNT];
        Win32GetInputFileLocation(State, true, InputPlaybackIndex, sizeof(Filename), Filename);
        State->PlaybackHandle = CreateFileA(Filename, GENERIC_READ,
                                            NULL, NULL,
                                            OPEN_EXISTING,0, NULL);
#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = State->TotalSize;
        SetFilePointerEx(State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
#endif

        CopyMemory(State->AppMemoryBlock, Buffer->MemoryBlock, (size_t)State->TotalSize);
    }
}

internal void Win32EndInputPlayback(win32_state *State)
{
    CloseHandle(State->PlaybackHandle);
    State->InputPlaybackIndex = 0;
}

internal void Win32RecordInput(win32_state *State, app_input *Input)
{
    DWORD BytesWritten;
    WriteFile(State->RecordingHandle, Input, sizeof(*Input), &BytesWritten, 0);
}

internal void Win32PlaybackInput(win32_state *State, app_input *Input)
{
    DWORD BytesRead = 0;
    if (ReadFile(State->PlaybackHandle, Input, sizeof(*Input), &BytesRead, 0))
    {
        if (BytesRead == 0)
        {
            // this is the end, go back to the beginning
            i32 InputPlaybackIndex = State->InputPlaybackIndex;
            Win32EndInputPlayback(State);
            Win32BeginInputPlayback(State, InputPlaybackIndex);
            ReadFile(State->PlaybackHandle, Input, sizeof(*Input), &BytesRead, 0);
        }
    }
}

internal void Win32ProcessPendingMessages(win32_state *State, app_controller_input *KeyboardController)
{
    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
        case WM_QUIT:
        {
            GlobalRunning = false;
        }
        break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            u32 VKCode = (u32)Message.wParam;
            b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
            b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
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
                        GlobalPause = !GlobalPause;
                }
                else if (VKCode == 'L')
                {
                    if (IsDown)
                    {
                        if (State->InputPlaybackIndex == 0)
                        {
                            if (State->InputRecordingIndex == 0)
                            {
                                Win32BeginRecordingInput(State, 1);
                            }
                            else
                            {
                                Win32EndRecordingInput(State);
                                Win32BeginInputPlayback(State, 1);
                            }
                        }
                        else
                        {
                            Win32EndInputPlayback(State);
                        }
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
                else if (VKCode == VK_F11)
                {
                    if (IsDown)
                    {
                        if (Message.hwnd)
                        {
                            Win23ToggleFullScreen(Message.hwnd);
                        }
                    }
                }
            }
            b32 AltKeyWasDown = Message.lParam & (1 << 29);
            if ((VKCode == VK_F4) && AltKeyWasDown)
                GlobalRunning = false;
        }
        break;
        default:
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        break;
        }
    }

    // for some reason these messages don't seem go get caught above
    Win32ProcessKeyboardMessage(&KeyboardController->MouseButtons[0],
                                 GetKeyState(VK_LBUTTON) & (1 << 15));
    Win32ProcessKeyboardMessage(&KeyboardController->MouseButtons[1],
                                 GetKeyState(VK_MBUTTON) & (1 << 15));
    Win32ProcessKeyboardMessage(&KeyboardController->MouseButtons[2],
                                 GetKeyState(VK_RBUTTON) & (1 << 15));
    Win32ProcessKeyboardMessage(&KeyboardController->MouseButtons[3],
                                 GetKeyState(VK_XBUTTON1) & (1 << 15));
    Win32ProcessKeyboardMessage(&KeyboardController->MouseButtons[4],
                                 GetKeyState(VK_XBUTTON1) & (1 << 15));
}



// timing stuff
inline LARGE_INTEGER Win32GetWallClockTime(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline r32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    return (r32)(End.QuadPart - Start.QuadPart) /
        (r32)GlobalPerfCounterFrequency;
}



// the actual stuff
int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CmdLine, int CmdShow)
{
    win32_state Win32State = {};

    LARGE_INTEGER GlobalPerfCounterFrequencyResult;
    QueryPerformanceFrequency(&GlobalPerfCounterFrequencyResult);
    GlobalPerfCounterFrequency = GlobalPerfCounterFrequencyResult.QuadPart;

    Win32GetEXEFilename(&Win32State);

    char SourceAppCodeDLLFullPath[WIN32_STATE_FILENAME_COUNT];
    Win32BuildEXEPathFilename(&Win32State, "app.dll",
                               sizeof(SourceAppCodeDLLFullPath), SourceAppCodeDLLFullPath);

    char TempAppCodeDLLFullPath[WIN32_STATE_FILENAME_COUNT];
    Win32BuildEXEPathFilename(&Win32State, "app_temp.dll",
                               sizeof(TempAppCodeDLLFullPath), TempAppCodeDLLFullPath);

    UINT DesiredSchedulerMiliseconds = 1;
    b32 SleepIsGranuler = (timeBeginPeriod(DesiredSchedulerMiliseconds) == TIMERR_NOERROR);

    Win32LoadXInput();

    GlobalShowCursor = true;

    WNDCLASSA WindowClass = {0};

    //sWin32ResizeDIBSection(&GlobalBackBuffer, 960, 540);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "app";
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);

    if (RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(0, // WS_EX_TOPMOST | WS_EX_LAYERED,
                            WindowClass.lpszClassName, "app", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

        if (Window)
        {
            Win32InitOpenGl(Window);
            HDC RefreshDC = GetDC(Window);
            win32_sound_output SoundOutput = {};
            ReleaseDC(Window, RefreshDC);
            i32 MonitorRefreshFrequency = 60;
            i32 Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            if (Win32RefreshRate > 1)
            {
                MonitorRefreshFrequency = GetDeviceCaps(RefreshDC, VREFRESH);
                // how reliable? don't know
            }
            r32 AppRefreshFrequency = MonitorRefreshFrequency / 2.0f;
            r32 TargetSecondsPerFrame = 1.0f / AppRefreshFrequency;

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(i16) * 2;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.SecondaryBufferSize = SoundOutput.BytesPerSample * SoundOutput.SamplesPerSecond;
            // todo
            //  actually compute this variance and figure out lowest reasonable value is
            SoundOutput.SafetyBytes = (i32)(((r32)SoundOutput.SamplesPerSecond *
                (r32)SoundOutput.BytesPerSample / AppRefreshFrequency) / 3.0f);

            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(&SoundOutput);

            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            GlobalRunning = true;

            i16 *Samples = (i16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
                                                   MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            b32 SoundIsPlaying = false;

#if BUILD_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes((u64)1);
#else
            LPVOID BaseAddress = 0;
#endif

            app_memory AppMemory = {};
            AppMemory.PermanentStorageSize = Megabytes((u64)64);
            AppMemory.TransientStorageSize = Megabytes((u64)250);
            // note
            //  reduced from 1gb to 250mb, may cause problems later
            AppMemory.DEBUGPlatformFreeEntireFile = DEBUGPlatformFreeEntireFile;
            AppMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            AppMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            // todo
            //  handle various memory footprints using system metrics;
            //  use MEM_LARGE_PAGES and call AdjustTokenPrivileges when
            //  not in windows XP?
            Win32State.TotalSize = AppMemory.PermanentStorageSize + AppMemory.TransientStorageSize;
            Win32State.AppMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
                                                      MEM_RESERVE | MEM_COMMIT, // MEM_LARGE_PAGES?
                                                      PAGE_READWRITE);
            AppMemory.PermanentStorageBytes = Win32State.AppMemoryBlock;
            AppMemory.TransientStorageBytes =
                (u8 *)AppMemory.PermanentStorageBytes + AppMemory.PermanentStorageSize;

            // note
            //  this might not work on ths computer because of only
            //  8gb of ram

            for (i32 ReplayIndex = 0; ReplayIndex < ArrayCount(Win32State.ReplayBuffers); ReplayIndex++)

            {
                // todo
                //  recording still takes a while on startup,
                //  find out what windows is doing that takes
                //  that long and if it is possible to defer
                //  or speed it up
                win32_replay_buffer *ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];
                Win32GetInputFileLocation(&Win32State, false, ReplayIndex, sizeof(ReplayBuffer->Filename),
                                          ReplayBuffer->Filename);

                ReplayBuffer->FileHandle = CreateFileA(ReplayBuffer->Filename, GENERIC_READ | GENERIC_WRITE,
                                                       0, NULL,
                                                       CREATE_ALWAYS, 0, NULL);
                LARGE_INTEGER MaxSize = {};
                MaxSize.QuadPart = Win32State.TotalSize;
                ReplayBuffer->MemoryMap = CreateFileMapping(ReplayBuffer->FileHandle, NULL, PAGE_READWRITE,
                                                            MaxSize.HighPart, MaxSize.LowPart, NULL);
                ReplayBuffer->MemoryBlock = MapViewOfFile(ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0,
                                                          (size_t)Win32State.TotalSize);
                if (ReplayBuffer->MemoryBlock)
                {
                    //
                }
                else
                {
                    // todo
                    //  diagostic and support for low memory systems
                }
            }

            if (Samples && AppMemory.PermanentStorageBytes)
            {

                app_input Input[2] = {};
                app_input *NewInput = &Input[0];
                app_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClockTime();
                LARGE_INTEGER FlipWallClock = Win32GetWallClockTime();

                i32 DEBUGTimeMarkerIndex = 0;
                debug_win32_time_marker DEBUGTimeMarkers[30] = {};

                DWORD AudioLatencyBytes = 0;
                r32 AudioLatencySeconds = 0.0f;
                b32 SoundIsValid = false;

                win32_app_code App = Win32LoadAppCode(SourceAppCodeDLLFullPath, TempAppCodeDLLFullPath);

                i64 LastCycleCount = __rdtsc();
                while (GlobalRunning)
                {
                    NewInput->dtForFrame = TargetSecondsPerFrame;

                    FILETIME NewDLLWriteTime = GetLastFileWriteTime(SourceAppCodeDLLFullPath);
                    if (CompareFileTime(&NewDLLWriteTime, &App.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadAppCode(&App);
                        App = Win32LoadAppCode(SourceAppCodeDLLFullPath, TempAppCodeDLLFullPath);
                    }

                    app_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    app_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    app_controller_input ZeroController = {};
                    *NewKeyboardController = ZeroController;
                    NewKeyboardController->IsConnected = true;
                    for (i32 ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                         ButtonIndex++)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

                    if (!GlobalPause)
                    {
                        POINT MousePoint;
                        GetCursorPos(&MousePoint);
                        ScreenToClient(Window, &MousePoint);
                        NewKeyboardController->MouseX = MousePoint.x;
                        NewKeyboardController->MouseY = MousePoint.y;
                        NewKeyboardController->MouseZ = 0;
                        // note
                        //  can I do this processing elsewhere?
                        // todo
                        //  support mouse wheel

                        DWORD MaxControllerCount = XUSER_MAX_COUNT;
                        if (MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                        {
                            MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
                        }
                        for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount;
                             ControllerIndex++)
                        {
                            DWORD OurControllerIndex = ControllerIndex + 1;
                            app_controller_input *OldController =
                                GetController(OldInput, OurControllerIndex);
                            app_controller_input *NewController =
                                GetController(NewInput, OurControllerIndex);

                            XINPUT_STATE ControllerState;
                            if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) // absolutely windows
                            {
                                // the controller is plugged in
                                NewController->IsConnected = true;
                                NewController->IsAnalog = OldController->IsAnalog;

                                XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                                NewController->StickAverageX = Win32ProcessXInputStickValue(
                                    Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                NewController->StickAverageY = Win32ProcessXInputStickValue(
                                    Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                                if ((NewController->StickAverageX != 0.0f) ||
                                    (NewController->StickAverageX != 0.0f))
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

                                r32 Threshold = 0.5f;
                                Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0,
                                                                &OldController->ActionLeft, 1, &NewController->ActionLeft);
                                Win32ProcessXInputDigitalButton((NewController->StickAverageX > Threshold) ? 1 : 0,
                                                                &OldController->ActionRight, 1, &NewController->ActionRight);
                                Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0,
                                                                &OldController->ActionUp, 1, &NewController->ActionUp);
                                Win32ProcessXInputDigitalButton((NewController->StickAverageY > Threshold) ? 1 : 0,
                                                                &OldController->ActionDown, 1, &NewController->ActionDown);

                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown,
                                                                XINPUT_GAMEPAD_A, &NewController->ActionDown);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight,
                                                                XINPUT_GAMEPAD_B, &NewController->ActionRight);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft,
                                                                XINPUT_GAMEPAD_X, &NewController->ActionLeft);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp,
                                                                XINPUT_GAMEPAD_Y, &NewController->ActionUp);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder,
                                                                XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder,
                                                                XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start,
                                                                XINPUT_GAMEPAD_START, &NewController->Start);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back,
                                                                XINPUT_GAMEPAD_BACK, &NewController->Back);
                            }
                            else
                            {
                                // controller unavaliable
                                NewController->IsConnected = false;
                            }
                        }

                        thread_context Thread;

                        if (Win32State.InputRecordingIndex)
                            Win32RecordInput(&Win32State, NewInput);

                        if (Win32State.InputPlaybackIndex)
                            Win32PlaybackInput(&Win32State, NewInput);

                        // here

                        if (App.UpdateAndRender)
                            App.UpdateAndRender(&Thread, &AppMemory, NewInput);

                        LARGE_INTEGER AudioWallClock = Win32GetWallClockTime();
                        r32 FromBeginToAudioSeconds =
                            Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                        {
                            /*
                                note:
                                Here's how the sound output computation works:
                                    A Safety value is defined that is the nunber of
                                samples we think the app update loop may vary by
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

                                // and also i don't really understand this
                            */
                            if (!SoundIsValid)
                            {
                                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                                SoundIsValid = true;
                            }

                            DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
                                               SoundOutput.SecondaryBufferSize;

                            DWORD ExpectedSoundBytesPerFrame =
                                (i32)((r32)(SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) /
                                        AppRefreshFrequency);
                            r32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                            DWORD ExpectedBytesUntilFlip =
                                (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame) *
                                        (r32)ExpectedSoundBytesPerFrame);

                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

                            DWORD SafeWriteCursor = WriteCursor;
                            if (SafeWriteCursor < PlayCursor)
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;

                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;
                            b32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

                            DWORD TargetCursor = 0;
                            if (AudioCardIsLowLatency)
                            {
                                TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
                            }
                            else
                            {
                                TargetCursor =
                                    (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
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

                            app_sound_buffer SoundBuffer = {};
                            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;

                            if (App.GetSoundSamples)
                                App.GetSoundSamples(&Thread, &AppMemory, &SoundBuffer);
#if BUILD_INTERNAL
                            debug_win32_time_marker *Marker = &DEBUGTimeMarkers[DEBUGTimeMarkerIndex];
                            Marker->OutputPlayCursor = PlayCursor;
                            Marker->OutputWriteCursor = WriteCursor;
                            Marker->OutputLocation = ByteToLock;
                            Marker->OutputByteCount = BytesToWrite;
                            Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                            DWORD UnwrappedWriteCursor = WriteCursor;
                            if (UnwrappedWriteCursor < PlayCursor)
                                UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                            AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                            AudioLatencySeconds =
                                ((r32)AudioLatencyBytes / (r32)SoundOutput.BytesPerSample) /
                                (r32)SoundOutput.SamplesPerSecond;

#if 0
                            char TextBuffer[256];
                            sprintf_s (
                                TextBuffer, sizeof(TextBuffer),
                                "BTL: %u\tTC: %u\tBTW: %uPC: %u\tWC: %u\tDELTA: %u(%fs)\n",
                                ByteToLock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds
                            );
                            OutputDebugStringA(TextBuffer);
#endif

#endif
                            Win32FillSoundBuffer(&SoundBuffer, &SoundOutput, ByteToLock, BytesToWrite);
                        }
                        else
                        {
                            SoundIsValid = false;
                        }

                        if (!SoundIsPlaying)
                        {
                            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                            SoundIsPlaying = true;
                        }

                        LARGE_INTEGER WorkCounter = Win32GetWallClockTime();
                        r32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                        r32 SecondsElapsedForFrame = WorkSecondsElapsed;
                        if (SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            if (SleepIsGranuler)
                            {
                                DWORD SleepMiliseconds =
                                    (DWORD)((SecondsElapsedForFrame - TargetSecondsPerFrame));
                                if (SleepMiliseconds > 0)
                                    Sleep(SleepMiliseconds);
                            }

                            r32 TestSecondsElapsedForFrame =
                                Win32GetSecondsElapsed(LastCounter, Win32GetWallClockTime());

                            if (TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                // todo
                                //  log missed sleep here
                            }

                            while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame =
                                    Win32GetSecondsElapsed(LastCounter, Win32GetWallClockTime());
                            }
                        }
                        else
                        {
                            // missed frame rate!
                            // todo
                            //  logging
                        }

                        LARGE_INTEGER EndCounter = Win32GetWallClockTime();
                        r32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                        LastCounter = EndCounter;

                        win32_window_dimensions Dimension = Win32GetWindowDimensions(Window);

                        // Win32SyncDisplay was here

                        HDC DeviceContext = GetDC(Window);
                        Win32DisplayBuffer(DeviceContext , Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);

                        FlipWallClock = Win32GetWallClockTime();

#if BUILD_INTERNAL
                        {
                            if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                            {
                                Assert(DEBUGTimeMarkerIndex < ArrayCount(DEBUGTimeMarkers));
                                debug_win32_time_marker *Marker = &DEBUGTimeMarkers[DEBUGTimeMarkerIndex];
                                Marker->FlipPlayCursor = PlayCursor;
                                Marker->FlipWriteCursor = WriteCursor;
                            }
                        }

#endif
                        app_input *Temp = NewInput;
                        NewInput = OldInput;
                        OldInput = Temp;

#if 0
                        u64 EndCycleCount = __rdtsc();
                        u64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

                        r32 FramesPerSecond = 0.0f;
                        r32 MCyclesPerFrame = (r32)CyclesElapsed / 1000000.0f;

                        char FPSBuffer[256];
                        sprintf_s(FPSBuffer, sizeof(FPSBuffer), "%f ms/f\t\t%f f/s\t\t%f Mc/f\n", MSPerFrame, FramesPerSecond, MCyclesPerFrame);
                        OutputDebugStringA(FPSBuffer);
#endif

#if BUILD_INTERNAL
                        ++DEBUGTimeMarkerIndex;
                        if (DEBUGTimeMarkerIndex == ArrayCount(DEBUGTimeMarkers))
                            DEBUGTimeMarkerIndex = 0;
#endif
                    }
                }
            }
            else
            {
                // todo
                //  logging
            }
        }
        else
        {
            // todo
            //  logging
        }
    }
    else
    {
        // todo
        //  logging
    }

    return 0;
}
