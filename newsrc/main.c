//note: maybe do something like Win32ErrorAndDie for logging

#define WINDOW_TITLE          "Application"
#define PROGRAM_FILENAME      "app"
#define DEFAULT_WINDOW_WIDTH   1280
#define DEFAULT_WINDOW_HEIGHT  720

#include <windows.h>

#include "lingo.h"
#include "platform.h"

global platform Platorm;

typedef struct _win32_timer {
    LARGE_INTEGER CountsPerSecond;
    LARGE_INTEGER FrameBegin;
    b32           SleepIsGranular;
} win32_timer;

typedef struct _win32_app_code {
    HMODULE              DLL;
    FILETIME             LastDLLWriteTime;
    app_update_callback *Update;
} win32_app_code;

internal b32 Win32LoadAppCode(win32_app_code *Code,
                              char *DLLPath, char *TempDLLPath)
{
    b32 Success = 1;
    
    while (true) {
        if (CopyFileA(DLLPath, TempDLLPath, FALSE))
            break;
    }

    Code->DLL = LoadLibraryA(TempDLLPath);
    Code->LastDLLWriteTime = GetLastFileWriteTime(DLLPath);
    if (!Code->DLL) {
        Success = 0;
        return Success;
    }

    Code->Update = (app_update_callback *)GetProcAddress(Code->DLL, "Update");
    if (!Code->Update) {
        Code->Update = AppUpdateStub;
        Success = 0;
        return Success;
    }

    return Success;
}

void CatStrings(size_t SourceACount, char *SourceA,
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
    CatStrings(OnePastLastSlash - ExecutablePath,
               ExecutablePath, StringLenght(Filename), Filename,
               DestCount, Dest);
}

int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CmdLine, int CmdShow)
{
    // timing
    win32_timer Timer;
    QueryPerformanceFrequency(&Timer.CountsPerSecond);

    // get paths for dlls filename for executable and working directory
    char ExecutablePath  [MAX_PATH];
    char AppDLLPath      [MAX_PATH];
    char TempAppDLLPath  [MAX_PATH];
    char WorkingDirectory[MAX_PATH];
    {
        // path for the executable
        DWORD SizeofFileName =
            GetModuleFileNameA(0, ExecutablePath, sizeof(ExecutablePath));
        char *OnePastLastSlash = ExecutablePath;
        for (char *Scan = ExecutablePath; *Scan; ++Scan) {
            if (*Scan == '\\')
                OnePastLastSlash = Scan + 1;
        }

        // paths for the dll's
        Win32BuildEXEPathFilename(AppDLLPath, sizeof(AppDLLPath), "app.dll",
                                  OnePastLastSlash, ExecutablePath);
        Win32BuildEXEPathFilename(TempAppDLLPath, sizeof(TempAppDLLPath), "temp_app.dll",
                                  OnePastLastSlash, ExecutablePath);

        // working dir
        GetCurrentDirectory(sizeof(WorkingDirectory), WorkingDirectory);
    }

    WNDCLASS WindowClass = {0}; {
        WindowClass.style = CS_HREDRAW | CS_VREDRAW;
        WindowClass.lpfnWndProc = Win32WindowProc;
            //todo: Win32WindowProc
        WindowClass.hInstance = Instance;
        WindowClass.lpszClassName = "ApplicationWindowClass";
        WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    }

    if(!RegisterClass(&WindowClass)) {
        //note: ERROR!! Window class failed to registrate
        //todo: logging
    }

    HWND WindowHandle = CreateWindow("ApplicationWindowClass", WINDOW_TITLE,
                                      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                      DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT,
                                      0, 0, Instance, 0);
    
    if(!WindowHandle)
    {
        //note: ERROR!! Window failed to be created
        //todo: logging
    }

    // load app code
    win32_app_code AppCode = {0}; {
        if(!Win32LoadAppCode(&AppCode)) {
            //note: ERROR!! App code failed to load
            //todo: logging
        }
    }

    // sound
    {
        //todo
    }

    // get refresh rate
    f32 MonitorRefreshRate = 60.f; {
        DEVMODEA DeviceMode = {0};
        if(EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &DeviceMode)) {
            MonitorRefreshRate = (float)DeviceMode.dmDisplayFrequency;
        }
    }

    return 0;
}
