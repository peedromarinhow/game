//note: maybe do something like Win32ErrorAndDie for logging

#define WINDOW_TITLE          "Application"
#define PROGRAM_FILENAME      "app"
#define DEFAULT_WINDOW_WIDTH   1280
#define DEFAULT_WINDOW_HEIGHT  720

#include <windows.h>

#include "lingo.h"
#include "platform.h"

#include "code.c"
#include "timing.c"

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

internal void Win32ProcessKeyboardMessage(button_state *State, b32 IsDown) {
    if (State->EndedDown != IsDown) {
        State->EndedDown = IsDown;
        ++State->HalfTransitionCount;
    }
}

internal void Win32ProcessPendingMessages(platform *Platform) {
    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
        switch (Message.message) {
            case WM_QUIT: {
                Platform->Running = 0;
                break;
            }
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                b32 AltKeyWasDown = Message.lParam & (1 << 29);
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                u32 VKCode = (u32)Message.wParam;
                if (WasDown != IsDown) {
                    //note: macro for these?
                    if (VKCode == 'A')
                        Win32ProcessKeyboardMessage(&Platform->KeyboardButtons[1], IsDown);

                    if (VKCode == VK_F11)
                        if (Message.hwnd)
                            Win23ToggleFullScreen(Message.hwnd);
                }

                if ((VKCode == VK_F4) && AltKeyWasDown)
                    Platform->Running = false;

                break;
            }
            default: {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
                break;
            }
        }
    }

    // for some reason these messages don't seem go get caught above
    Win32ProcessKeyboardMessage(&Platform->MouseButtons[0], GetKeyState(VK_LBUTTON)  & (1 << 15));
    Win32ProcessKeyboardMessage(&Platform->MouseButtons[1], GetKeyState(VK_MBUTTON)  & (1 << 15));
    Win32ProcessKeyboardMessage(&Platform->MouseButtons[2], GetKeyState(VK_RBUTTON)  & (1 << 15));
}

int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CmdLine, int CmdShow)
{
    // timing
    win32_timer Timer;
    QueryPerformanceFrequency(&Timer.CountsPerSecond);
    Timer.SleepIsGranular = (timeBeginPeriod(1) == TIMERR_NOERROR);

    // get paths for dlls filename for executable and working directory
    char ExecutablePath  [MAX_PATH];
    char WorkingDirectory[MAX_PATH];
    char AppDLLPath      [MAX_PATH];
    char TempAppDLLPath  [MAX_PATH];
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

   // initialize platform  
    platform Platform = {}; {
        Platform.ExecutablePath       = ExecutablePath;
        Platform.WorkingDirectoryPath = WorkingDirectory;

        Platform.Running = 1;
        //note: other fields are updated pre frame
    }

    ShowWindow(WindowHandle, CmdShow);
    UpdateWindow(WindowHandle);

    while (Platform.Running) {
        Win32BeginFrameTiming(&Timer);
        Win32ProcessPendingMessages(&Platform)

        // updates
        // stuff is going to go in here

        Platform.dtForFrame = Win32EndFrameTiming(&Timer);
    }

    return 0;
}
