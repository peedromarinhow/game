//note: maybe do something like Win32ErrorAndDie for logging
//note: this is very inspired by https://github.com/ryanfleury/app_template

#define WINDOW_TITLE          "Application"
#define PROGRAM_FILENAME      "app"
#define DEFAULT_WINDOW_WIDTH   1280
#define DEFAULT_WINDOW_HEIGHT  720

#include <windows.h>

#if BUILD_INTERNAL
#include <stdio.h>
#endif

#include "lingo.h"
#include "platform.h"
#include "memory.h"

#include "win32_paths.c"
#include "win32_timing.c"
#include "win32_code.c"
#include "win32_sound.c"
#include "win32_opengl.c"
#include "win32_input.c"

global platform GlobalPlatform;

internal void Win32ToggleFullScreen(HWND Window) {
    localpersist WINDOWPLACEMENT WindowPosition = {sizeof(WindowPosition)};
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

//note: hopefully the demiurge is having fun
internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                                  WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;

    b32 WasDown = (lParam & (1 << 30)) != 0;
    b32 IsDown  = (lParam & (1 << 31)) == 0;

    if (Message == WM_QUIT  ||
        Message == WM_CLOSE ||
        Message == WM_DESTROY)
    {
        GlobalPlatform.Running = 0;
    }
    //
    // MOUSE
    else
    if (Message == WM_MOUSEHWHEEL) {
        GlobalPlatform.dMouseWheel = 10;
        OutputDebugStringA("mouse\n");
    }
    else
    if (Message == WM_LBUTTONDOWN) {
        Win32ProcessButtonMessage(&GlobalPlatform.MouseLeft, 1);
    }
    else
    if (Message == WM_LBUTTONUP) {
        Win32ProcessButtonMessage(&GlobalPlatform.MouseLeft, 0);
    }
    else
    if (Message == WM_RBUTTONDOWN) {
        Win32ProcessButtonMessage(&GlobalPlatform.MouseRight, 1);
    }
    else
    if (Message == WM_RBUTTONUP) {
        Win32ProcessButtonMessage(&GlobalPlatform.MouseRight, 0);
    }
    else
    if (Message == WM_MBUTTONDOWN) {
        Win32ProcessButtonMessage(&GlobalPlatform.MouseMiddle, 1);
    }
    else
    if (Message == WM_MBUTTONUP) {
        Win32ProcessButtonMessage(&GlobalPlatform.MouseMiddle, 0);
    }
    else
    if (Message == WM_SETCURSOR) {
        SetCursor(LoadCursorA(0, IDC_ARROW));
    }
    //
    // KEYBOARD
    else
    if (Message == WM_SYSKEYDOWN ||
        Message == WM_SYSKEYUP   ||
        Message == WM_KEYDOWN    ||
        Message == WM_KEYUP)
    {
        b32 AltKeyWasDown = lParam & (1 << 29);
        //todo: do this AltKeyWasDown differently
        u64 VKCode = wParam;
        if (WasDown != IsDown) {
            if (VKCode == VK_F11) {
                if (IsDown && Window)
                    Win32ToggleFullScreen(Window);
            }
                //note:
                // send this to the platform and let it decide how to handle
                // fullscreen switching?
            else
            if (VKCode == VK_UP)
                Win32ProcessButtonMessage(&GlobalPlatform.KeyboardUp, IsDown);
            else
            if (VKCode == VK_DOWN)
                Win32ProcessButtonMessage(&GlobalPlatform.KeyboardDown, IsDown);
            else
            if (VKCode == VK_LEFT)
                Win32ProcessButtonMessage(&GlobalPlatform.KeyboardLeft, IsDown);
            else
            if (VKCode == VK_RIGHT)
                Win32ProcessButtonMessage(&GlobalPlatform.KeyboardRight, IsDown);
            else
            if (VKCode == VK_F4) {
                if (AltKeyWasDown) GlobalPlatform.Running = 0;
            }
        }
    }
    else
    if (Message == WM_CHAR) {
        u64 CharacterInput = wParam;
        if(CharacterInput >= 32        && 
           CharacterInput != 127       &&
           CharacterInput != VK_RETURN &&
           CharacterInput != VK_ESCAPE)
        {
            GlobalPlatform.CharacterInput = CharacterInput;
        }
    }
    //
    else
    if (Message == WM_PAINT) {
        PAINTSTRUCT Paint;
        BeginPaint(Window, &Paint);
        EndPaint(Window, &Paint);
    }
    else {
        Result = DefWindowProcA(Window, Message, wParam, lParam);
    }

    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CmdLine, int CmdShow)
{
    // timing
    win32_timer Timer = {0}; {
        QueryPerformanceFrequency(&Timer.CountsPerSecond);
        Timer.SleepIsGranular = (timeBeginPeriod(1) == TIMERR_NOERROR);
        Timer.TargetFPS       = 0;
        //note: 0 for no fps capping
    }

    // get paths for dlls filename for executable and working directory
    char ExecutablePath  [256];
    char WorkingDirectory[256];
    char AppDLLPath      [256];
    char TempAppDLLPath  [256];
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

    //note: global platform seems to be inevitable because windows
    // it's great
    GlobalPlatform.ExecutablePath       = ExecutablePath;
    GlobalPlatform.WorkingDirectoryPath = WorkingDirectory;
#if BUILD_INTERNAL
    LPVOID BaseAddress = (LPVOID)SafeTruncateU64(Terabytes((u64)1));
#else
    LPVOID BaseAddress = 0;
#endif
    GlobalPlatform.Memory.Size = Megabytes((u64)64);
    GlobalPlatform.Memory.Contents = VirtualAlloc(BaseAddress, (size_t)GlobalPlatform.Memory.Size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!GlobalPlatform.Memory.Contents) {
        //note: ERROR!! Could not allocate memory for the app
        //todo: logging
    }
    //note: other fields are updated per frame

    WNDCLASS WindowClass = {0}; {
        WindowClass.style = CS_HREDRAW | CS_VREDRAW;
        WindowClass.lpfnWndProc = Win32MainWindowCallback;
            //todo: Win32WindowProc
        WindowClass.hInstance = Instance;
        WindowClass.lpszClassName = "ApplicationWindowClass";
        WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    }

    if(!RegisterClass(&WindowClass)) {
        //note: ERROR!! Window class failed to registrate
        //todo: logging
    }

    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, WINDOW_TITLE,
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 
                                  0, 0, Instance, 0);
    
    if(!Window) {
        //note: ERROR!! Window failed to be created
        //todo: logging
    }

    // load app code
    win32_app_code AppCode = {0}; {
        if(!Win32LoadAppCode(&AppCode, AppDLLPath, TempAppDLLPath)) {
            //note: ERROR!! App code failed to load
            //todo: logging
        }
    }

    // get refresh rate
    f32 MonitorRefreshRate = 60.0f; {
        DEVMODEA DeviceMode = {0};
        if(EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &DeviceMode)) {
            MonitorRefreshRate = (float)DeviceMode.dmDisplayFrequency;
        }
    }

    //todo: sound

    GlobalPlatform.Running = 1;

    Win32InitOpenGl(Window);

    AppCode.Init(&GlobalPlatform);

    while (GlobalPlatform.Running) {
        Win32BeginFrameTiming(&Timer);
         
         // process messages
         {
            MSG Message;
            while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
         }

        // get window dimensions
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            GlobalPlatform.WindowSize.x = ClientRect.right  - ClientRect.left;
            GlobalPlatform.WindowSize.y = ClientRect.bottom - ClientRect.top;
        }

        // update input for stuff that doesn't come trough the messages, see Win32ProcessPendingMessages
        {
            POINT MousePoint;
            GetCursorPos(&MousePoint);
            ScreenToClient(Window, &MousePoint);
            GlobalPlatform.MousePos.x = MousePoint.x;
            GlobalPlatform.MousePos.y = MousePoint.y;
                //todo: mouse wheel
        }

        //todo: sound

        // update
        {
            AppCode.Update(&GlobalPlatform);

            // opengl swapbuffers
            {
                HDC DeviceContext = GetDC(Window);
                SwapBuffers(DeviceContext);
                ReleaseDC(Window, DeviceContext);
            }
        }

        Win32UpdateAppCode(&AppCode, AppDLLPath, TempAppDLLPath);
        GlobalPlatform.dtForFrame = Win32EndFrameTiming(&Timer);

#if BUILD_INTERNAL
        char FPSBuffer[256];
        sprintf_s(FPSBuffer, sizeof(FPSBuffer), "%f ms/f\n", GlobalPlatform.dtForFrame);
        OutputDebugStringA(FPSBuffer);
#endif
    }

    ShowWindow(Window, SW_HIDE);

    Win32UnloadAppCode(&AppCode);
    // Win32DeinitOpenGl();

    return 0;
}
