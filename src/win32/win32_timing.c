#include "lingo.h"

typedef struct _win32_timer {
    LARGE_INTEGER CountsPerSecond;
    LARGE_INTEGER FrameBegin;
    b32           SleepIsGranular;
    u32           TargetFPS;
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
    r32 dtForFrame = ((f32)(Win32GetCounterTime().QuadPart - Timer->FrameBegin.QuadPart)) /
                      (f32)(Timer->CountsPerSecond.QuadPart);
    if (Timer->SleepIsGranular && (Timer->TargetFPS > 0)) {
        Sleep((DWORD)((1.0f/Timer->TargetFPS) - dtForFrame)*1000);
    }
    return dtForFrame;
}
