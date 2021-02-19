#include "lingo.h"
#include "platform.h"

typedef struct _win32_timer {
    LARGE_INTEGER FrameBegin;
    LARGE_INTEGER FrameEnd;
    LARGE_INTEGER CountsPerSecond;
} win32_timer;

inline void Win32BeginFrameTiming(win32_timer *Timer) {
    QueryPerformanceCounter(&Timer->FrameBegin);
}

inline r32 Win32EndFrameTiming(win32_timer *Timer, platform *Platform) {
    QueryPerformanceCounter(&Timer->FrameEnd);
    return ((r32)(Timer->FrameEnd.QuadPart - Timer->FrameBegin.QuadPart) / (r32)(Timer->CountsPerSecond.QuadPart));
}
