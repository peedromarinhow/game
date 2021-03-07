#include "lingo.h"

PLATFORM_LOAD_OPENGL_FUNCTION(Win32LoadOpenGlFunction) {
    void *Result = (void *)wglGetProcAddress(FunctionName);
    if(!Result                ||
        Result == (void *)0x1 ||
        Result == (void *)0x2 ||
        Result == (void *)0x3 ||
        Result == (void *) - 1)
    {
        return 0;
    }
    else {
        return Result;
    }
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
    if (wglMakeCurrent(WindowDC, OpenGLRC)) {
        //note: sucess!
    }
    else {
        Win32ReportErrorAndDie("FATAL ERROR", "wglMakeCurrent failed");
    }
    ReleaseDC(Window, WindowDC);
}
