//note: maybe do something like Win32ErrorAndDie for logging
//note: this is very inspired by https://github.com/ryanfleury/app_template

#define WINDOW_TITLE          "Application"
#define PROGRAM_FILENAME      "app"
#define DEFAULT_WINDOW_WIDTH   1280
#define DEFAULT_WINDOW_HEIGHT  720

#include <windows.h>
#include <gl/gl.h>

#include "lingo.h"
#include "platform.h"

#include "paths.c"
#include "timing.c"
#include "code.c"

global b32 GlobalRunning;

internal void Win23ToggleFullScreen(HWND Window) {
    localper WINDOWPLACEMENT WindowPosition = {sizeof(WindowPosition)};
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
            case WM_CLOSE:
            case WM_DESTROY: 
            case WM_QUIT: {
                GlobalRunning = 0;
                break;
            }
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                u32 VKCode = (u32)Message.wParam;
                if (WasDown != IsDown) {
                    if (VKCode == 'A')
                        Win32ProcessKeyboardMessage(&Platform->KeyboardButtons[1], IsDown);

                    if (VKCode == VK_F11 && IsDown)
                        if (Message.hwnd)
                            Win23ToggleFullScreen(Message.hwnd);
                }

                b32 AltKeyWasDown = Message.lParam & (1 << 29);
                if ((VKCode == VK_F4) && AltKeyWasDown)
                    GlobalRunning = 0;
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
    Win32ProcessKeyboardMessage(&Platform->MouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
    Win32ProcessKeyboardMessage(&Platform->MouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
    Win32ProcessKeyboardMessage(&Platform->MouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
}

/*internal b32 Win32InitOpenGl(HWND Window) {/
    b32 Success = 0;
    HDC WindowDC = GetDC(Window);

    //todo: cColorBits supposed to exclude alpha bits?
    PIXELFORMATDESCRIPTOR PixelFormatDescriptor =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    i32 PixelFormat = ChoosePixelFormat(WindowDC, &PixelFormatDescriptor);
    if (PixelFormat) {
        SetPixelFormat(WindowDC, PixelFormat, &PixelFormatDescriptor);
        HGLRC RenderContext = wglCreateContext(WindowDC);
        wglMakeCurrent(WindowDC, RenderContext);
        Success = 1;
    }

    ReleaseDC(Window, WindowDC);

    return Success;
}*/

internal void Win32InitOpenGl(HWND Window) {
    HDC WindowDC = GetDC(Window);

    // todo wtf
    //  cColorBits supposed to exclude alpha bits?
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

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

internal void Win32DumbRenderSomething (HWND WindowHandle) {
    glViewport(0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
    // todo
    //  remove this
    GLuint TextureHandle = 0;
    static b32 Init = 0;
    if (!Init) {
        glGenTextures(1, &TextureHandle);
        Init = 1;
    }
    glBindTexture(GL_TEXTURE_2D, TextureHandle);
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
    HDC DeviceContext = GetDC(WindowHandle);
    SwapBuffers(DeviceContext);
    ReleaseDC(WindowHandle, DeviceContext);
}

//note: hope the demiurge is having fun
internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                                  WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    if (Message == WM_CLOSE || Message == WM_DESTROY || Message == WM_QUIT) {
        GlobalRunning = 0;
        Result = 0;
    }
    else if (Message == WM_PAINT) {
        PAINTSTRUCT Paint;
        Win32DumbRenderSomething(Window);
        EndPaint(Window, &Paint);
    }
    else {
        Result = DefWindowProcA(Window, Message, WParam, LParam);
    }
    return Result;
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

   // initialize platform (up here because windows is great)
    platform Platform = {0}; {
        Platform.ExecutablePath       = ExecutablePath;
        Platform.WorkingDirectoryPath = WorkingDirectory;
        //note: other fields are updated per frame
    }
    // it's great

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

    Win32InitOpenGl(WindowHandle);

    ShowWindow(WindowHandle, CmdShow);
    UpdateWindow(WindowHandle);

    GlobalRunning = 1;

    while (GlobalRunning == 1) {
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

            Win32DumbRenderSomething(WindowHandle);

            // update fullscreen condition if necessary
            if (WasFullscreen != Platform.Fullscreen)
                Win23ToggleFullScreen(WindowHandle);
        }

        Win32UpdateAppCode(&AppCode, AppDLLPath, TempAppDLLPath);

        Platform.dtForFrame = Win32EndFrameTiming(&Timer);

        GlobalRunning = 0;
    }

    ShowWindow(WindowHandle, SW_HIDE);

    Win32UnloadAppCode(&AppCode);
    // Win32DeinitOpenGl();

    return 0;
}
