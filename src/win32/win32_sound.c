#include "lingo.h"

typedef struct _win32_sound_output {
    i32  SamplesPerSecond;
    i32  BytesPerSample;
    u32  RunningSampleIndex;
    DWORD SecondaryBufferSize;
    DWORD SafetyBytes;
} win32_sound_output;