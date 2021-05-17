#include "lingo.h"
#include "platform.h"
#include "memory.h"

//todo: put this on some string file and make it better
internal void DebugCatStrings(size_t SourceACount, char *SourceA,
                size_t SourceBCount, char *SourceB,
                size_t DestCount, char *Dest)
{
    for (size_t Index = 0; Index < SourceACount; Index++) {
        *Dest++ = *SourceA++;
    }
    for (size_t Index = 0; Index < SourceBCount; Index++) {
        *Dest++ = *SourceB++;
    }

    *Dest++ = '\0';
}

internal void Win32BuildEXEPathFilename(char *Dest, i32 DestCount, char *Filename,
                                        char *OnePastLastSlash, char *ExecutablePath)
{
    DebugCatStrings(OnePastLastSlash - ExecutablePath,
                    ExecutablePath, StringLenght(Filename), Filename,
                    DestCount, Dest);
}

//note: all this is basically _stolen_ from ryan's platform layer

//todo: varargs for formats on these two functions
internal PLATFORM_REPORT_ERROR(Win32ReportError) {
    MessageBoxA(0, ErrorMessage, Title, MB_OK);
}

internal PLATFORM_REPORT_ERROR_AND_DIE(Win32ReportErrorAndDie) {
    MessageBoxA(0, ErrorMessage, Title, MB_OK);
    _Exit(1);
}

internal PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory) {
    void *Result = VirtualAlloc(0, (size_t)Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!Result) {
        Win32ReportError("MEMORY ALLOCATION ERROR", "Could not allocate");
    }
    return Result;
}

internal PLATFORM_FREE_MEMORY(Win32FreeMemory) {
    if (Data)
        VirtualFree(Data, sizeof(Data), MEM_RELEASE);
}

internal PLATFORM_FREE_FILE(Win32FreeFile) {
    if (File.Data)
        VirtualFree(File.Data, File.Size, MEM_RELEASE);
}

//note: basically copied from ryan's
internal PLATFORM_LOAD_FILE(Win32LoadFile) {
    file Result;
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ | GENERIC_WRITE,
                                    0, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE) {
        DWORD ReadBytes = GetFileSize(FileHandle, 0);
        if (ReadBytes != INVALID_FILE_SIZE) {
            void *ReadData = VirtualAlloc(NULL, (size_t)ReadBytes + 1,
                                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            DWORD BytesRead = 0;

            ReadFile(FileHandle, ReadData, ReadBytes, &BytesRead, NULL);

            ((u8 *)ReadData)[ReadBytes] = 0;

            Result.Data = ReadData;
            Result.Size = (u64)BytesRead;
        }
        CloseHandle(FileHandle);
    }
    return Result;
}

internal PLATFORM_FREE_FILE_FROM_ARENA(Win32FreeFileFromArena) {
    PopFromArena(Arena, File.Size);
}

internal PLATFORM_LOAD_FILE_TO_ARENA(Win32LoadFileToArena) {
    file Result;
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ | GENERIC_WRITE,
                                    0, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE) {
        DWORD ReadBytes = GetFileSize(FileHandle, 0);
        if (ReadBytes != INVALID_FILE_SIZE) {
            void *ReadData = PushToArena(Arena, ReadBytes + 1);
            DWORD BytesRead = 0;

            ReadFile(FileHandle, ReadData, ReadBytes, &BytesRead, NULL);

            ((u8 *)ReadData)[ReadBytes] = 0;

            Result.Data = ReadData;
            Result.Size = (u64)BytesRead;
        }
        CloseHandle(FileHandle);
    }
    return Result;
}

internal PLATFORM_WRITE_FILE(Win32WriteFile) {
    HANDLE FileHandle = {0};
    if (Append) {
        FileHandle = CreateFileA(Filename, FILE_APPEND_DATA,
                                 FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
        SetFilePointer(FileHandle, 0, 0, FILE_END);
    }
    else {
        FileHandle = CreateFileA(Filename, GENERIC_READ | GENERIC_WRITE,
                                 0, 0, CREATE_ALWAYS, 0, 0);
    }
    
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        void *DataToWrite     = Data;
        DWORD DataToWriteSize = Size;//(Size > 0)? Size : sizeof(Data);
        DWORD BytesWritten    = 0;

        WriteFile(FileHandle, DataToWrite, DataToWriteSize, &BytesWritten, NULL);
        
        CloseHandle(FileHandle);
    }
    else {
        Win32ReportError("ERROR", "Could not save to file");
        //Win32ReportError("ERROR", "Could not save to file\"%s\"\n", Filename);
    }
}

PLATFORM_GET_DIR_FILENAMES(Win32GetDirFilenames) {
    WIN32_FIND_DATA FindData = {0};
    HANDLE          FindHandle = FindFirstFileA(Dir, &FindData);
    u32             NoFiles = 0;
    if (FindHandle != INVALID_HANDLE_VALUE) {
        do {
            NoFiles++;
        } while (FindNextFileA(FindHandle, &FindData));
    }

    c8 **Filenames = Win32AllocateMemory(NoFiles + 1 * sizeof(c8 *));
    u32  FileIndex = 0;
    FindHandle = FindFirstFileA(Dir, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE) {
        do {
            Filenames[FileIndex] = Win32AllocateMemory(sizeof(FindData.cFileName));
            CopyMemory(Filenames[FileIndex], FindData.cFileName, sizeof(FindData.cFileName));
            FileIndex++;
        } while (FindNextFileA(FindHandle, &FindData));
        FindClose(FindHandle);
    }

    Filenames[NoFiles] = 0;

    return (file_group){Filenames, NoFiles};
}

PLATFORM_GRAPHICS_CLEAR(Win32Clear) {
    glLoadIdentity();
    glViewport(0, 0, TargetDim.w, TargetDim.h);
    
    r32 a = 2.0f/TargetDim.w;
    r32 b = 2.0f/TargetDim.h;
    r32 Proj[] = {
        a, 0, 0, 0,
        0, b, 0, 0,
        0, 0, 1, 0,
       -1,-1, 0, 1
    };

    glLoadMatrixf(Proj);
    glClearColor(Color.r, Color.g, Color.b, Color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

PLATFORM_GRAPHICS_CLIP(Win32Clip) {
    glScissor(ClipRect.x, ClipRect.y, ClipRect.w, ClipRect.h);
}

PLATFORM_GRAPHICS_RASTER_RECT(Win32RasterRect) {
    glBegin(GL_POLYGON); {
        glColor4f(Color.r, Color.g, Color.b, Color.a);
        glVertex2f(Rect.x,          Rect.y + Rect.h);
        glVertex2f(Rect.x + Rect.w, Rect.y + Rect.h);
        glVertex2f(Rect.x + Rect.w, Rect.y);
        glVertex2f(Rect.x,          Rect.y);
    } glEnd();
}

PLATFORM_GRAPHICS_RASTER_TEXTURE_RECT(Win32RasterTextureRect) {
    glBindTexture(GL_TEXTURE_2D, Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        rv2 TextureDim  = rv2_(Texture.w, Texture.h);

        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);

        glTexCoord2f(Rect.x / TextureDim.w, Rect.y / TextureDim.h);
        glVertex2f(Pos.x, Pos.y + Rect.h);

        glTexCoord2f((Rect.x + Rect.w) / TextureDim.w, Rect.y /TextureDim.h);
        glVertex2f(Pos.x + Rect.w, Pos.y + Rect.h);
        
        glTexCoord2f((Rect.x + Rect.w) / TextureDim.w, (Rect.y + Rect.h) / TextureDim.h);
        glVertex2f(Pos.x + Rect.w, Pos.y);

        glTexCoord2f(Rect.x / TextureDim.w, (Rect.y + Rect.h) / TextureDim.h);
        glVertex2f(Pos.x, Pos.y);
    } glEnd();
    glDisable(GL_TEXTURE_2D);
}

PLATFORM_GRAPHICS_ENABLE(Win32Enable) {
    glEnable(Opt);
}

PLATFORM_GRAPHICS_DISABLE(Win32Disable) {
    glDisable(Opt);
}

PLATFORM_GRAPHICS_GEN_AND_BIND_AND_LOAD_TEXTURE(Win32GenAndBindAndLoadTexture) {
    glGenTextures(1, &Texture->Id);
    glBindTexture(GL_TEXTURE_2D, Texture->Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Texture->w, Texture->h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, Image->Data);
}

PLATFORM_GRAPHICS_BLEND_FUNC(Win32BlendFunc) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

internal void Win32ToggleFullScreen(HWND Window) {
    persist WINDOWPLACEMENT WindowPosition = {sizeof(WindowPosition)};
    //note:
    //  copied from https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    //  by Raymond Chen
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if (GetWindowPlacement(Window, &WindowPosition) &&
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
    else {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &WindowPosition);
        SetWindowPos(Window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

inline FILETIME GetFileLastWriteTime(c8 *Filename) {
    FILETIME Result = {0};
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
        Result = Data.ftLastWriteTime;
    return Result;
}

internal void Win32InitOpenGl(HWND Window) {
    HDC WindowDC = GetDC(Window);

    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
        32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 0, PFD_MAIN_PLANE, 0, 0,
        0, 0
    };

    i32 SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat),   &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);

    HGLRC OpenGLRC = wglCreateContext(WindowDC);
    if (wglMakeCurrent(WindowDC, OpenGLRC));
        //note: sucess!
    else
        Win32ReportErrorAndDie("FATAL ERROR", "wglMakeCurrent failed");
    ReleaseDC(Window, WindowDC);
}

inline iv2 Win32GetWindowDim(HWND Window) {
    iv2 Result = {0};
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.w = ClientRect.right  - ClientRect.left;
    Result.h = ClientRect.bottom - ClientRect.top;
    return Result;
}

inline rv2 Win32GetMousePos(HWND Window, iv2 WindowDimensions) {
    rv2 Result = {0};
    POINT MousePoint;
    GetCursorPos(&MousePoint);
    ScreenToClient(Window, &MousePoint);
    Result.x =  MousePoint.x;
    Result.y = -MousePoint.y + WindowDimensions.y;
    return Result;
}

typedef struct _win32_timer {
    u64 FrameBegin;
    u64 CounterFrequency;
    u64 FrameEnd;
    u64 FrameDuration;
} win32_timer;

inline u64 Win32GetTime() {
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result.QuadPart;
}

inline u64 Win32GetCounterFrequency() {
    LARGE_INTEGER Result;
    QueryPerformanceFrequency(&Result);
    return Result.QuadPart;
}

inline r32 Win32GetFrameTime(win32_timer *t, f32 TargetSecondsPerFrame) {
    t->FrameEnd      = Win32GetTime();
    t->FrameDuration = (t->FrameEnd - t->FrameBegin);
    t->FrameBegin    = Win32GetTime();

    r32 dt = (f32)t->FrameDuration / (r32)t->CounterFrequency;

    // f64 TargetSeconds = (dt / 1000.0);
    // i64 TargetCounts  = (i64)(TargetSeconds * t->CounterFrequency);
    // i64 CountsToWait  = TargetCounts - t->FrameDuration;
    
    // LARGE_INTEGER WaitBegin;
    // LARGE_INTEGER WaitEnd;
    
    // QueryPerformanceCounter(&WaitBegin);
    
    // while(CountsToWait > 0)
    // {
    //     DWORD MsToSleep = (DWORD)(1000.0 * ((f64)(CountsToWait) / t->CounterFrequency));
    //     if(MsToSleep > 0)
    //         Sleep(MsToSleep);
        
    //     QueryPerformanceCounter(&WaitEnd);
    //     CountsToWait -= WaitEnd.QuadPart - WaitBegin.QuadPart;
    //     WaitBegin = WaitEnd;
    // }

    return dt;
}

typedef struct _win32_app_code {
    HMODULE              DLL;
    FILETIME             LastDLLWriteTime;
    app_init_callback   *Init;
    app_reload_callback *Reload;
    app_update_callback *Update;
    app_deinit_callback *Deinit;
} win32_app_code;

internal b32 Win32LoadAppCode(win32_app_code *Code,
                              c8 *DLLPath, c8 *TempDLLPath)
{
    b32 Success = 1;
    while (TRUE) {
        if (CopyFileA(DLLPath, TempDLLPath, FALSE))
            break;
    }

    Code->DLL = LoadLibraryA(TempDLLPath);
    Code->LastDLLWriteTime = GetFileLastWriteTime(DLLPath);
    if (!Code->DLL) {
        Success = 0;
        return Success;
    }

    Code->Update = (app_update_callback *)GetProcAddress(Code->DLL, "Update");
    Code->Reload = (app_reload_callback *)GetProcAddress(Code->DLL, "Reload");
    Code->Init   = (app_init_callback *)  GetProcAddress(Code->DLL, "Init");
    Code->Deinit = (app_deinit_callback *)GetProcAddress(Code->DLL, "Deinit");

    if (!Code->Update || !Code->Reload || !Code->Init || !Code->Deinit) {
        Code->Update = AppUpdateStub;
        Code->Reload = AppReloadStub;
        Code->Init   = AppInitStub;
        Code->Deinit = AppDeinitStub;
        Success = 0;
        return Success;
    }

    return Success;
}

internal void Win32UnloadAppCode(win32_app_code *Code)
{
    if(Code->DLL)
        FreeLibrary(Code->DLL);
    Code->DLL = 0;
    Code->Update = AppUpdateStub;
}

internal b32 Win32UpdateAppCode(win32_app_code *Code, c8 *DLLPath, c8 *TempDLLPath) {
    FILETIME LastDLLWriteTime = GetFileLastWriteTime(DLLPath);
    if(CompareFileTime(&LastDLLWriteTime, &Code->LastDLLWriteTime)) {
        Win32UnloadAppCode(Code);
        Win32LoadAppCode(Code, DLLPath, TempDLLPath);
        return 1;
    }
    return 0;
}
