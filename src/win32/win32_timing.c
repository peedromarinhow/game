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

inline void Win32EndFrameTiming(win32_timer *Timer, r64 MillisecondsPerFrame) {
    i64 ElapsedCounts = Win32GetCounterTime().QuadPart - Timer->FrameBegin.QuadPart;
    f64 DesiredSecondsPerFrame = MillisecondsPerFrame / 10000.0;
    u64 DesiredCounts = (i64)(DesiredSecondsPerFrame * Timer->CountsPerSecond.QuadPart);
    i64 CountsToSleep = DesiredCounts - ElapsedCounts;

    if (MillisecondsPerFrame != 0) {
    }
}

internal void
W32_TimerEndFrame(W32_Timer *timer, f64 milliseconds_per_frame)
{
    LARGE_INTEGER end_frame;
    QueryPerformanceCounter(&end_frame);
    
    f64 desired_seconds_per_frame = (milliseconds_per_frame * 1000.0);
    i64 elapsed_counts = end_frame.QuadPart - timer->begin_frame.QuadPart;
    i64 desired_counts = (i64)(desired_seconds_per_frame * timer->counts_per_second.QuadPart);
    i64 counts_to_wait = desired_counts - elapsed_counts;
    
    LARGE_INTEGER start_wait;
    LARGE_INTEGER end_wait;
    
    QueryPerformanceCounter(&start_wait);
    
    while(counts_to_wait > 0)
    {
        if(timer->sleep_is_granular)
        {
            DWORD milliseconds_to_sleep = (DWORD)(1000.0 * ((f64)(counts_to_wait) / timer->counts_per_second.QuadPart));
            if(milliseconds_to_sleep > 0)
            {
                Sleep(milliseconds_to_sleep);
            }
        }
        
        QueryPerformanceCounter(&end_wait);
        counts_to_wait -= end_wait.QuadPart - start_wait.QuadPart;
        start_wait = end_wait;
    }
}
