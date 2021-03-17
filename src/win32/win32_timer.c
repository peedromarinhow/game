#include "lingo.h"
#include "platform.h"

inline u64 Win32GetTime() {
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result.QuadPart;
}
