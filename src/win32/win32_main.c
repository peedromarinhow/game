//note: this is very inspired by https://github.com/ryanfleury/app_template

#include <windows.h>
#include <gl/gl.h>

#define WINDOW_TITLE          "Application"
#define PROGRAM_FILENAME      "app"
#define DEFAULT_WINDOW_WIDTH   800
#define DEFAULT_WINDOW_HEIGHT  900
#define MOUSE_POSITION_WHEN_OUT_OF_WINDOW 1

#include "lingo.h"
#include "platform.h"

#include "win32_internal.c"
#include "win32_utils.c"
#include "win32_code.c"
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

int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CmdLine, i32 CmdShow)
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

    WNDCLASS WindowClass = {
        .style         = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc   = Win32MainWindowCallback,
        .hInstance     = Instance,
        .lpszClassName = WINDOW_TITLE,
        .hCursor       = LoadCursor(0, IDC_ARROW)
    };
    if (!RegisterClass(&WindowClass))
         Win32ReportErrorAndDie("ERROR!!", "Window class failed to registrate");

    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, WINDOW_TITLE,
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT,
                                  0, 0, Instance, 0);
    
    if (!Window)
         Win32ReportErrorAndDie("ERROR!!", "Window failed to be created");

    /* initializing paltform */
    platform Platform = {0}; {
        Platform.ExecutablePath       = ExecutablePath;
        Platform.WorkingDirectoryPath = WorkingDirectory;
        Platform.Memory.Size          = Megabytes((u64)64);
        Platform.Memory.Contents      = Win32AllocateMemory(Platform.Memory.Size);
        if (!Platform.Memory.Contents)
            Win32ReportErrorAndDie("ERROR!!", "Could not allocate memory for the app");

        /* functions provided by platform */
        Platform.Api.AllocateMemory    = Win32AllocateMemory;
        Platform.Api.FreeMemory        = Win32FreeMemory;
        Platform.Api.LoadFile          = Win32LoadFile;
        Platform.Api.FreeFile          = Win32FreeFile;
        Platform.Api.LoadFileToArena   = Win32LoadFileToArena;
        Platform.Api.FreeFileFromArena = Win32FreeFileFromArena;
        Platform.Api.WriteFile         = Win32WriteFile;
        Platform.Api.GetDirFilenames   = Win32GetDirFilenames;
        Platform.Api.ReportError       = Win32ReportError;
        Platform.Api.ReportErrorAndDie = Win32ReportErrorAndDie;

        Platform.Api.Clear             = Win32Clear;
        Platform.Api.Clip              = Win32Clip;
        Platform.Api.RasterRect        = Win32RasterRect;
        Platform.Api.RasterTextureRect = Win32RasterTextureRect;
        Platform.Api.Enable            = Win32Enable;
        Platform.Api.Disable           = Win32Disable;
        Platform.Api.GenAndBindAndLoadTexture = Win32GenAndBindAndLoadTexture;
        Platform.Api.BlendFunc         = Win32BlendFunc;

        Platform.WindowDim = Win32GetWindowDim(Window);
        Platform.MousePos  = Win32GetMousePos(Window, Platform.WindowDim);
    };

    /* load app code */
    win32_app_code AppCode = {0}; {
        if (!Win32LoadAppCode(&AppCode, AppDLLPath, TempAppDLLPath))
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
    
    HDC GlDeviceContext = GetDC(Window);
    Win32InitOpenGl(Window);

    b32 mPosOutOfWindow = 0;

    //note:
    // this "GlobalRunning" is just for the window closing messages
    // that come exclusively through "Win32MainWindowCallback"
    GlobalRunning = &Platform.Running;
   *GlobalRunning = 1;

    AppCode.Init(&Platform);

    u64 FrameBegin       = Win32GetTime();
    u64 CounterFrequency = Win32GetCounterFrequency();
    u64 FrameEnd         = 0;
    u64 FrameDuration    = 0;

    win32_timer Timer = {
        .FrameBegin       = Win32GetTime(),
        .CounterFrequency = Win32GetCounterFrequency(),
        .FrameEnd         = 0,
        .FrameDuration    = 0
    };

    u64 VKCodeTable[256] = {
        [VK_UP] = plat_KEYB_UP,
        [VK_DOWN] = plat_KEYB_DOWN,
        [VK_LEFT] = plat_KEYB_LEFT,
        [VK_RIGHT] = plat_KEYB_RIGHT,
        [VK_HOME] = plat_KEYB_HOME,
        [VK_END] = plat_KEYB_END,
        [VK_PRIOR] = plat_KEYB_PG_UP,
        [VK_NEXT] = plat_KEYB_PG_DOWN,
        [VK_BACK] = plat_KEYB_BACK,
        [VK_DELETE] = plat_KEYB_DELETE,
        [VK_RETURN] = plat_KEYB_RETURN,
        [VK_TAB] = plat_KEYB_TAB,
        [VK_CONTROL] = plat_KEYB_CTRL,
        [VK_SHIFT] = plat_KEYB_SHIFT,
        [VK_MENU] = plat_KEYB_ALT
    };

    while (Platform.Running) {
        /* Process pending messages */ {
            MSG Message;
            u32 LastButton = plat_KEY_NONE;
            while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                Platform.Buttons[plat_KEYBEV_CHAR] = 0;

                b8  WasDown =  (Message.lParam & (1 << 30)) != 0;
                b8  IsDown  = !(Message.lParam & (1 << 31));
                u64 VKCode  =   Message.wParam;

                if (Message.message == WM_KEYDOWN ||
                    Message.message == WM_KEYUP)
                {
                    if (VKCode == VK_F11) {
                        if (IsDown && Window)
                            Win32ToggleFullScreen(Window); //note: send this to the platform?
                    }

                    if (VKCode == VK_F4) {
                        if (Message.lParam & (1 << 29))
                            Platform.Running = 0;
                    }

                    if (WasDown != IsDown) {
                        Platform.Buttons[VKCodeTable[VKCode]] = IsDown;
                    }

                    BYTE KeyboardState[256];
                    GetKeyboardState(KeyboardState);
                    WORD Chars;
                    if (ToAscii(Message.wParam, (Message.lParam >> 16) & 0xFF, KeyboardState, &Chars, 0) == 1) {
                        Platform.Char = (u8)Chars;
                        Platform.Buttons[plat_KEYBEV_CHAR] = 1;
                        if (Platform.Buttons[plat_KEYB_CTRL])
                            Platform.Char = Message.wParam;    
                    }
                    
                    TranslateMessage(&Message);
                }
                else // windows' stuff
                if (Message.message == WM_PAINT) {
                    PAINTSTRUCT Paint;
                    BeginPaint(Window, &Paint);
                    EndPaint(Window, &Paint);
                }
                else {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
            }
        }

        //todo: sound

        /* update */ {
            Platform.WindowDim = Win32GetWindowDim(Window);
            Platform.MousePos  = Win32GetMousePos(Window, Platform.WindowDim);

            AppCode.Update(&Platform);

            wglSwapLayerBuffers(GlDeviceContext, WGL_SWAP_MAIN_PLANE);
        }
        
        if (Win32UpdateAppCode(&AppCode, AppDLLPath, TempAppDLLPath))
            AppCode.Reload(&Platform);

        Platform.dtForFrame = Win32GetFrameTime(&Timer, 1.f/MonitorRefreshRate);
    }

    AppCode.Deinit(&Platform);

    Win32UnloadAppCode(&AppCode);
    ShowWindow(Window, SW_HIDE);
    ReleaseDC(Window, GlDeviceContext);

    return 0;
}
