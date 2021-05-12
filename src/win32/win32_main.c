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

    while (Platform.Running) {
        /* Process pending messages */ {
            //note:
            // since there is no "WM_MOUSE_DID_NOT_MOVE" message, assume that it didn't and
            // if it acually did, then update
            MSG Message;
            i16 dmWheel = 0;
            Win32ProcessEventMessage(&Platform.Buttons[plat_KEYMEV_MOVED], 0);
            Win32ProcessEventMessage(&Platform.Buttons[plat_KEYBEV_CHAR],  0);
            while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                //note: WM_QUIT WM_CLOSE WM_DESTROY are caught in WindowProc
                /* mouse */
                if (Message.message == WM_MOUSEWHEEL) {
                    dmWheel = HIWORD(Message.wParam);
                }
                else
                if (Message.message == WM_MOUSEMOVE){
                    Win32ProcessEventMessage(&Platform.Buttons[plat_KEYMEV_MOVED], 1);
                }
                else
                if (Message.message == WM_LBUTTONDOWN)
                    Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYM_LEFT], 1);
                else
                if (Message.message == WM_LBUTTONUP)
                    Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYM_LEFT], 0);
                else
                if (Message.message == WM_RBUTTONDOWN)
                    Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYM_RIGHT], 1);
                else
                if (Message.message == WM_RBUTTONUP)
                    Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYM_RIGHT], 0);
                else
                if (Message.message == WM_MBUTTONDOWN)
                    Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYM_MIDDLE], 1);
                else
                if (Message.message == WM_MBUTTONUP)
                    Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYM_MIDDLE], 0);
                else
                if (Message.message == WM_SETCURSOR)
                    SetCursor(LoadCursorA(0, IDC_ARROW));

                b32 AltKeyWasDown =  Message.lParam & (1 << 29);
                b32 WasDown       = (Message.lParam & (1 << 30)) != 0;
                b32 IsDown        = (Message.lParam & (1 << 31)) == 0;
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
                        if (VKCode == VK_UP)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_UP], IsDown);
                        else
                        if (VKCode == VK_DOWN)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_DOWN], IsDown);
                        else
                        if (VKCode == VK_LEFT)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_LEFT], IsDown);
                        else
                        if (VKCode == VK_RIGHT)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_RIGHT], IsDown);
                        else
                        if (VKCode == VK_HOME)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_HOME], IsDown);
                        else
                        if (VKCode == VK_END)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_END], IsDown);
                        else
                        if (VKCode == VK_PRIOR)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_PG_UP], IsDown);
                        else
                        if (VKCode == VK_NEXT)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_PG_DOWN], IsDown);
                        else
                        if (VKCode == VK_BACK)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_BACK], IsDown);
                        else
                        if (VKCode == VK_DELETE)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_DELETE], IsDown);
                        else
                        if (VKCode == VK_RETURN)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_RETURN], IsDown);
                        else
                        if (VKCode == VK_TAB)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_TAB], IsDown);
                        else
                        if (VKCode == VK_CONTROL)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_CTRL], IsDown);
                        else
                        if (VKCode == VK_SHIFT)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_SHIFT], IsDown);
                        else
                        if (VKCode == VK_MENU)
                            Win32ProcessButtonMessage(&Platform.Buttons[plat_KEYB_ALT], IsDown);
                        else
                        if (VKCode == VK_F4) {
                            if (AltKeyWasDown) Platform.Running = 0;
                        }
                    }
                    if (Message.message == WM_KEYDOWN || Message.message == WM_SYSKEYDOWN) {
                        BYTE KeyboardState[256];
                        GetKeyboardState(KeyboardState);
                        WORD Chars;
                        if (ToAscii(Message.wParam, (Message.lParam >> 16) & 0xFF, KeyboardState, &Chars, 0) == 1) {
                            Platform.Char = (u8)Chars;
                            Win32ProcessEventMessage(&Platform.Buttons[plat_KEYBEV_CHAR], 1);
                        }

                        if (Platform.Buttons[plat_KEYB_CTRL].EndedDown)
                            Platform.Char = Message.wParam;
                    }

                    TranslateMessage(&Message);
                }
                // else
                // if (Message.message == WM_CHAR || Message.message == WM_UNICHAR) {
                //     Win32ProcessEventMessage(&Platform.Buttons[plat_KEYBEV_CHAR], 1);
                //     Platform.Char = Message.wParam;// WideCharToMultiByte(CP_UTF8, 0, (WCHAR *)Message.wParam, 1, Platform.Chars, 8, 0, 0);
                //     //note: i give up trying to get utf8 characters from windows
                // }
                /* windows' stuff */

                if (Message.message == WM_PAINT) {
                    PAINTSTRUCT Paint;
                    BeginPaint(Window, &Paint);
                    EndPaint(Window, &Paint);
                }
                else {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                Platform.dMouseWheel = dmWheel;
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

#if 0
//note: inline yer functionne
internal void Win32ProcessPendingMessages(HWND Window, platform *Platform) {
    //note:
    // since there is no "WM_MOUSE_DID_NOT_MOVE" message, assume that it didn't and
    // if it acually did, then update
    MSG Message;
    i16 dmWheel = 0;
    Win32ProcessEventMessage(&Platform->mMoved, 0);
    Win32ProcessEventMessage(&Platform->kChar,      0);
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
        //note: WM_QUIT WM_CLOSE WM_DESTROY are caught in WindowProc
        /* mouse */
        if (Message.message == WM_MOUSEWHEEL) {
            dmWheel = HIWORD(Message.wParam);
        }
        else
        if (Message.message == WM_MOUSEMOVE){
            Platform->mPos = Win32GetMousePos(Window, Platform->WindowDim);
            Win32ProcessEventMessage(&Platform->mMoved, 1);
        }
        else
        if (Message.message == WM_LBUTTONDOWN)
            Win32ProcessButtonMessage(&Platform->mLeft, 1);
        else
        if (Message.message == WM_LBUTTONUP)
            Win32ProcessButtonMessage(&Platform->mLeft, 0);
        else
        if (Message.message == WM_RBUTTONDOWN)
            Win32ProcessButtonMessage(&Platform->mRight, 1);
        else
        if (Message.message == WM_RBUTTONUP)
            Win32ProcessButtonMessage(&Platform->mRight, 0);
        else
        if (Message.message == WM_MBUTTONDOWN)
            Win32ProcessButtonMessage(&Platform->mMiddle, 1);
        else
        if (Message.message == WM_MBUTTONUP)
            Win32ProcessButtonMessage(&Platform->mMiddle, 0);
        else
        if (Message.message == WM_SETCURSOR)
            SetCursor(LoadCursorA(0, IDC_ARROW));

        /* keyboard */
        b32 AltKeyWasDown =  Message.lParam & (1 << 29);
        b32 WasDown       = (Message.lParam & (1 << 30)) != 0;
        b32 IsDown        = (Message.lParam & (1 << 31)) == 0;
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
                if (VKCode == VK_HOME)
                    Win32ProcessButtonMessage(&Platform->kHome, IsDown);
                else
                if (VKCode == VK_END)
                    Win32ProcessButtonMessage(&Platform->kEnd, IsDown);
                else
                if (VKCode == VK_BACK)
                    Win32ProcessButtonMessage(&Platform->kBack, IsDown);
                else
                if (VKCode == VK_DELETE)
                    Win32ProcessButtonMessage(&Platform->kDelete, IsDown);
                else
                if (VKCode == VK_RETURN)
                    Win32ProcessButtonMessage(&Platform->kReturn, IsDown);
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
                if (VKCode == VK_F4) {
                    if (AltKeyWasDown) Platform->Running = 0;
                }
            }

            if (Message.message == WM_SYSKEYDOWN ||
                Message.message == WM_KEYDOWN)
            {
                BYTE KeyboardState[256];
				GetKeyboardState(KeyboardState);
				WORD Chars;
				if (ToAscii(Message.wParam, (Message.lParam >> 16) & 0xFF, KeyboardState, &Chars, 0) == 1) {     
					Platform->Char = (u8)Chars;
                    Win32ProcessEventMessage(&Platform->kChar, 1);
				}else {
					Platform->Char = 0;
				}
                if (!IsPrintableChar(Platform->Char))
                    Platform->Char = Message.wParam;
            }

            TranslateMessage(&Message);
        }
        // else
        // if (Message.message == WM_CHAR) {
        //     WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)&Message.wParam, 1,
        //                        &Platform->Char, 1, 0, 0);
        // }
        else
        if (Message.message == WM_SIZE)
            Win32ProcessEventMessage(&Platform->WindowResized, 1);

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
        Platform->dmWheel = dmWheel;
        Platform->WasDown     = WasDown;
        Platform->IsDown      = IsDown;
    }
}
#endif
