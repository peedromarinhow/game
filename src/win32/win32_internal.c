#include "lingo.h"
#include "platform.h"

internal void Win32InternalLogFPS(r32 dtForFrame) {
    char FPSBuffer[256];
    sprintf_s(FPSBuffer, sizeof(FPSBuffer), "%f s/f\t%f ms/ts\n", dtForFrame, ((1.0/60.0) - dtForFrame));
    OutputDebugStringA(FPSBuffer);
}
