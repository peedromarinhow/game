#include "lingo.h"

typedef struct _win32_timer {
    LARGE_INTEGER CountsPerSecond;
    LARGE_INTEGER FrameBegin;
    b32           SleepIsGranular;
} win32_timer;

inline LARGE_INTEGER Win32GetCounterTime(void) {
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline void Win32BeginFrameTiming(win32_timer *Timer) {
    QueryPerformanceCounter(&Timer->FrameBegin);
}

inline f32 Win32EndFrameTiming(win32_timer *Timer) {
    return (f32)(Timer->FrameBegin.QuadPart - Win32GetCounterTime().QuadPart) /
           (f32) Timer->CountsPerSecond.QuadPart;
}
