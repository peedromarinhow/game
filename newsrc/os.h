#include "lingo.h"

typedef struct _os_state {
    // METADATA
    char *ExecutableFolderPath;
    char *ExecutableAbsolutePath;
    char *WorkingDirectoryPath;

    //note: previously globals
    b32 Running;
    b32 Paused;
    LPDIRECTSOUNDBUFFER SecondaryBuffer;
    i64 PerformanceCounterFrequency;
    b32 ShowCursor;
    WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };

} os_state;