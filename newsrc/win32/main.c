//note: maybe do something like Win32ErrorAndDie for logging

#define WINDOW_TITLE          "Application"
#define PROGRAM_FILENAME      "app"
#define DEFAULT_WINDOW_WIDTH   1280
#define DEFAULT_WINDOW_HEIGHT  720

#include <windows.h>
#include <gl/gl.h>

#include "lingo.h"
#include "platform.h"

#include "code.c"
#include "timing.c"
#include "paths.c"

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

internal void Win32InitOpenGl(HWND Window) {
    HDC WindowDC = GetDC(Window);

    // todo wtf
    //  cColorBits supposed to exclude alpha bits?
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {0};
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
    if (wglMakeCurrent(WindowDC, OpenGLRC)) {
        //note: success!
    }
    else {
        Assert(!"NOOOOOOOOOOOO!!");
        // invalid code path
    }
    ReleaseDC(Window, WindowDC);
}

internal void Win23ToggleFullScreen(HWND Window) {
    localper WINDOWPLACEMENT WindowPosition = { sizeof(WindowPosition) };
    // note
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
        if(!Win32LoadAppCode(&AppCode, AppDLLPath, TempAppDLLPath)) {
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

    Win32InitOpenGl(WindowHandle);

    ShowWindow(WindowHandle, CmdShow);
    UpdateWindow(WindowHandle);

    while (Platform.Running) {
        Win32BeginFrameTiming(&Timer);
        Win32ProcessPendingMessages(&Platform);

        // get window dimensions
        {
            RECT ClientRect;
            GetClientRect(WindowHandle, &ClientRect);
            Platform.WindowSize.X = ClientRect.right  - ClientRect.left;
            Platform.WindowSize.Y = ClientRect.bottom - ClientRect.top;
        }

        // update input for stuff that doesn't come trough the messages, see Win32ProcessPendingMessages
        {
            POINT MousePoint;
            GetCursorPos(&MousePoint);
            ScreenToClient(WindowHandle, &MousePoint);
            Platform.MousePos.X = MousePoint.x;
            Platform.MousePos.Y = MousePoint.y;
                //todo: mouse wheel
        }

        //todo: sound

        // call the app layer to update
        {
            b32 WasFullscreen = Platform.Fullscreen;

            AppCode.Update(&Platform);

            // update fullscreen condition if necessary
            if (WasFullscreen != Platform.Fullscreen)
                Win23ToggleFullScreen(WindowHandle)
        }

        Win32UpdateAppCode(AppCode, AppDLLPath, TempAppDLLPath);

        Platform.dtForFrame = Win32EndFrameTiming(&Timer);
    }

    ShowWindow(WindowHandle, SW_HIDE);

    Win32UnloadAppCode(AppCode);
    // Win32DeinitOpenGl();

    return 0;
}
