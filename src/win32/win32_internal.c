#include "lingo.h"
#include "platform.h"

internal void Win32InternalLogFPS(r32 dtForFrame, HWND Window, r32 TargetFPS) {
#if BUILD_INTERNAL
    char TimeBuffer[128];
    sprintf_s(TimeBuffer, sizeof(TimeBuffer), "%f s/f\t%f ms/ts\n", dtForFrame, ((1.0/TargetFPS) - dtForFrame)*1000);
    OutputDebugStringA(TimeBuffer);
    char FPSBuffer[64];
    sprintf_s(FPSBuffer, sizeof(FPSBuffer), "%.2f f/s", 1.0f/dtForFrame);
    SetWindowTextA(Window, FPSBuffer);
#endif
}
