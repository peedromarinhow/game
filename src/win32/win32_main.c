//note: this is very inspired by https://github.com/ryanfleury/app_template

#include <windows.h>
#include <gl/gl.h>

#if BUILD_INTERNAL
#include <stdio.h>
#endif

#include "options.h"
#include "lingo.h"
#include "platform.h"
#include "memory.h"

#include "win32_internal.c"
#include "win32_paths.c"
#include "win32_sound.c"
#include "win32_input.c"
#include "win32_timer.c"
#include "win32_utils.c"
#include "win32_code.c"     //almost aligned all!
#include "win32_opengl.c"

global b32 *GlobalRunning;

//note: hopefully the demiurge is having fun
internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                                  WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    if (Message == WM_QUIT  ||
        Message == WM_CLOSE ||
        Message == WM_DESTROY)
    {
        *GlobalRunning = 0;
    }
    else {
        Result = DefWindowProcA(Window, Message, wParam, lParam);
    }

    return Result;
}

internal void Win32ProcessPendingMessages(HWND Window, platform *Platform) {
    //note:
    // since there is no "WM_MOUSE_DID_NOT_MOVE" message, assume that it didn't and
    // if it acually did, then update
    //todo:
    // if possible, store the events as variables and dispatch at the end i.e.:
    // b32 MouseMoved = 0;
    // u64 KeyPressed = 0;
    MSG Message;
    i16 dMouseWheel = 0;
    Win32ProcessEventMessage(&Platform->MouseMoved, 0);
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
        //note: WM_QUIT WM_CLOSE WM_DESTROY are caught in WindowProc
        /* mouse */
        if (Message.message == WM_MOUSEWHEEL) {
            dMouseWheel = HIWORD(Message.wParam);
        }
        else
        if (Message.message == WM_MOUSEMOVE){
            Platform->MousePos = Win32GetMousePos(Window);
            Win32ProcessEventMessage(&Platform->MouseMoved, 1);
        }
        else
        if (Message.message == WM_LBUTTONDOWN)
            Win32ProcessButtonMessage(&Platform->MouseLeft, 1);
        else
        if (Message.message == WM_LBUTTONUP)
            Win32ProcessButtonMessage(&Platform->MouseLeft, 0);
        else
        if (Message.message == WM_RBUTTONDOWN)
            Win32ProcessButtonMessage(&Platform->MouseRight, 1);
        else
        if (Message.message == WM_RBUTTONUP)
            Win32ProcessButtonMessage(&Platform->MouseRight, 0);
        else
        if (Message.message == WM_MBUTTONDOWN)
            Win32ProcessButtonMessage(&Platform->MouseMiddle, 1);
        else
        if (Message.message == WM_MBUTTONUP)
            Win32ProcessButtonMessage(&Platform->MouseMiddle, 0);
        else
        if (Message.message == WM_SETCURSOR)
            SetCursor(LoadCursorA(0, IDC_ARROW));

        /* keyboard */
        b32 WasDown       = (Message.lParam & (1 << 30)) != 0;
        b32 IsDown        = (Message.lParam & (1 << 31)) == 0;
        b32 AltKeyWasDown = Message.lParam & (1 << 29);
        if (Message.message == WM_SYSKEYDOWN ||
            Message.message == WM_SYSKEYUP   ||
            Message.message == WM_KEYDOWN    ||
            Message.message == WM_KEYUP)
        {
            u64 VKCode = Message.wParam;
            if (WasDown != IsDown) {
                if (VKCode == VK_F11) {
                    if (IsDown && Window)
                        Win32ToggleFullScreen(Window);
                        //note:
                        // send this to the platform and let it decide how to handle
                        // fullscreen switching?
                }
                else
                if (VKCode == VK_CONTROL)
                    Win32ProcessButtonMessage(&Platform->kCtrl, IsDown);
                else
                if (VKCode == VK_SHIFT)
                    Win32ProcessButtonMessage(&Platform->kShift, IsDown);
                else
                if (VKCode == VK_MENU)
                    Win32ProcessButtonMessage(&Platform->kAlt, IsDown);
                else
                if (VKCode == VK_UP)
                    Win32ProcessButtonMessage(&Platform->kUp, IsDown);
                else
                if (VKCode == VK_DOWN)
                    Win32ProcessButtonMessage(&Platform->kDown, IsDown);
                else
                if (VKCode == VK_LEFT)
                    Win32ProcessButtonMessage(&Platform->kLeft, IsDown);
                else
                if (VKCode == VK_RIGHT)
                    Win32ProcessButtonMessage(&Platform->kRight, IsDown);
                else
                if (VKCode == VK_F4) {
                    if (AltKeyWasDown) Platform->Running = 0;
                }
            }
        }
        else
        if (Message.message == WM_CHAR) {
            u64 CharacterInput =  Message.wParam;
            if (CharacterInput >= 32        && 
                CharacterInput != 127       &&
                CharacterInput != VK_RETURN &&
                CharacterInput != VK_ESCAPE)
            {
                Platform->KeyboardCharacter = CharacterInput;
            }
        }

        /* windows' stuff */
        else
        if (Message.message == WM_PAINT) {
            PAINTSTRUCT Paint;
            BeginPaint(Window, &Paint);
            EndPaint(Window, &Paint);
        }
        else {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        Platform->dMouseWheel = dMouseWheel;
    }
}

int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CmdLine, int CmdShow)
{
    /* get paths for dlls filename for executable and working directory */
    c8 ExecutablePath  [256];
    c8 WorkingDirectory[256];
    c8 AppDLLPath      [256];
    c8 TempAppDLLPath  [256];
    {
        /* get path of the executable */
        DWORD SizeofFileName = GetModuleFileNameA(0, ExecutablePath, sizeof(ExecutablePath));
        c8 *OnePastLastSlash = ExecutablePath;
        for (c8 *Scan = ExecutablePath; *Scan; ++Scan) {
            if (*Scan == '\\')
                OnePastLastSlash = Scan + 1;
        }

        /* get paths for the dll's */
        Win32BuildEXEPathFilename(AppDLLPath, sizeof(AppDLLPath), "app.dll",
                                  OnePastLastSlash, ExecutablePath);
        Win32BuildEXEPathFilename(TempAppDLLPath, sizeof(TempAppDLLPath), "temp_app.dll",
                                  OnePastLastSlash, ExecutablePath);

        /* get working dir */ 
        GetCurrentDirectoryA(sizeof(WorkingDirectory), WorkingDirectory);
    }

    /* initializing paltform */
    platform Platform = {0}; {
        Platform.ExecutablePath       = ExecutablePath;
        Platform.WorkingDirectoryPath = WorkingDirectory;
        LPVOID BaseAddress = 0;
#if BUILD_INTERNAL
        BaseAddress = (LPVOID)SafeTruncateU64(Terabytes((u64)1));
#endif
        Platform.Memory.Size = Megabytes((u64)64);
        Platform.Memory.Contents = VirtualAlloc(BaseAddress, (size_t)Platform.Memory.Size,
                                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!Platform.Memory.Contents)
            Win32ReportErrorAndDie("ERROR!!", "Could not allocate memory for the app");

        /* functions provided by platform */
        Platform.MemAllocCallback          = Win32MemAlloc;
        Platform.MemFreeCallback           = Win32MemFree;
        Platform.FileLoadCallback          = Win32FileLoad;
        Platform.FileFreeCallback          = Win32FileFree;
        Platform.FileLoadArenaCallback     = Win32FileLoadArena;
        Platform.FileFreeArenaCallback     = Win32FileFreeArena;
        Platform.FileWriteCallback         = Win32FileWrite;
        Platform.ReportErrorCallback       = Win32ReportError;
        Platform.ReportErrorAndDieCallback = Win32ReportErrorAndDie;
        //note: other fields are updated every frame
    }

    WNDCLASS WindowClass = {0}; {
        WindowClass.style         = CS_HREDRAW | CS_VREDRAW;
        WindowClass.lpfnWndProc   = Win32MainWindowCallback;
        WindowClass.hInstance     = Instance;
        WindowClass.lpszClassName = WINDOW_TITLE;
        WindowClass.hCursor       = LoadCursor(0, IDC_ARROW);
    }

    if(!RegisterClass(&WindowClass))
        Win32ReportErrorAndDie("ERROR!!", "Window class failed to registrate");

    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, WINDOW_TITLE,
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 
                                  0, 0, Instance, 0);
    
    if(!Window)
        Win32ReportErrorAndDie("ERROR!!", "Window failed to be created");

    /* load app code */
    win32_app_code AppCode = {0}; {
        if(!Win32LoadAppCode(&AppCode, AppDLLPath, TempAppDLLPath))
            Win32ReportErrorAndDie("ERROR!!", "App code failed to load");
    }

    /* get refresh rate */
    f32 MonitorRefreshRate = 60.0f; {
        DEVMODEA DeviceMode = {0};
        if(EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &DeviceMode)) {
            MonitorRefreshRate = (float)DeviceMode.dmDisplayFrequency;
        }
    }

    //todo: sound

    Win32InitOpenGl(Window);

    //note:
    // this "GlobalRunning" is just for the window closing messages
    // that como exclusively through "Win32MainWindowCallback"
    GlobalRunning = &Platform.Running;
    *GlobalRunning = 1;
    AppCode.Init(&Platform);

    win32_timer Timer;
    QueryPerformanceFrequency(&Timer.CountsPerSecond);

    while (Platform.Running) {
        Win32BeginFrameTiming(&Timer);
        Win32ProcessPendingMessages(Window, &Platform);

        /* get window dimensions */ {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            Platform.WindowSize.x = ClientRect.right  - ClientRect.left;
            Platform.WindowSize.y = ClientRect.bottom - ClientRect.top;
        }

        //todo: sound

        /* update */ {
            if (MOUSE_POSITION_WHEN_OUT_OF_WINDOW)
                Platform.MousePos = Win32GetMousePos(Window);
            AppCode.Update(&Platform);
        }

        /* OpenGL */ {
            HDC DeviceContext = GetDC(Window);
            SwapBuffers(DeviceContext);
            ReleaseDC(Window, DeviceContext);
        }

        Win32UpdateAppCode(&AppCode, AppDLLPath, TempAppDLLPath);
        Platform.dtForFrame = Win32EndFrameTiming(&Timer, &Platform);

        Win32InternalLogFPS(Platform.dtForFrame, Window);

    }

    AppCode.Deinit(&Platform);

    ShowWindow(Window, SW_HIDE);
    Win32UnloadAppCode(&AppCode);

    return 0;
}
