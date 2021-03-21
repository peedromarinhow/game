#ifndef PLATFORM_H
#define PLATFORM_H

#include "lingo.h"
#include "maths.h"
#include "memory.h"

//note: these functions are to be implemented in each platform and passed by the app via struct platform

typedef struct _file {
    void *Data;
    u64   Size;
} file;

#define PLATFORM_ALLOCATE_MEMORY(Name) void *Name(u32 Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory_callback);

#define PLATFORM_FREE_MEMORY(Name) void Name(void *Data)
typedef PLATFORM_FREE_MEMORY(platform_free_memory_callback);

#define PLATFORM_LOAD_FILE(Name) file Name(c8 *Filename)
typedef PLATFORM_LOAD_FILE(platform_load_file_callback);

#define PLATFORM_FREE_FILE(Name) void Name(file File)
typedef PLATFORM_FREE_FILE(platform_free_file_callback);

#define PLATFORM_LOAD_FILE_TO_ARENA(Name) file Name(memory_arena *Arena, c8 *Filename)
typedef PLATFORM_LOAD_FILE_TO_ARENA(platform_load_file_to_arena_callback);

#define PLATFORM_FREE_FILE_FROM_ARENA(Name) void Name(memory_arena *Arena, file File)
typedef PLATFORM_FREE_FILE_FROM_ARENA(platform_free_file_from_arena_callback);

#define PLATFORM_WRITE_FILE(Name) void Name(void *Data, u64 Size, c8 *Filename)
typedef PLATFORM_WRITE_FILE(platform_write_file_callback);

#define PLATFORM_REPORT_ERROR(Name) void Name(c8 *Title, c8 *ErrorMessage)
typedef PLATFORM_REPORT_ERROR(platform_report_error_callback);

#define PLATFORM_REPORT_ERROR_AND_DIE(Name) void Name(c8 *Title, c8 *ErrorMessage)
typedef PLATFORM_REPORT_ERROR_AND_DIE(platform_report_error_and_die_callback);

typedef struct _button_state {
    i32 HalfTransitionCount;
    b32 EndedDown;
} button_state;

typedef struct _event_state {
    i32 HalfTransitionCount;
    b32 EndedHappening;
} event_state;

//note: this is how the platform and the app communicate with each other.
#define KEYBOARD_MAX_BUTTONS 7
#define MOUSE_MAX_BUTTONS    3
typedef struct _platform {
    /* metadata */
    c8 *ExecutablePath;
    c8 *WorkingDirectoryPath;

    /* options */
    b32 Fullscreen;
    b32 Running;
    iv2 WindowDimensions;
    r32 dtForFrame;
    b32 CtrlKeyWasDown;
    b32 ShiftKeyWasDown;
    b32 AltKeyWasDown;
    b32 WindowResized;

    /* mouse input */
    /* event_state */ b32 MouseMoved;
    i16 dMouseWheel;
    rv2 MousePos;
    /* button_state */ b32 MouseButtons[MOUSE_MAX_BUTTONS];
    /* button_state */ b32 MouseLeft;
    /* button_state */ b32 MouseRight;
    /* button_state */ b32 MouseMiddle;

    /* keyboard input */
    /* button_state */ b32 KeyboardButtons[KEYBOARD_MAX_BUTTONS];
    /* button_state */ b32 kUp;
    /* button_state */ b32 kDown;
    /* button_state */ b32 kLeft;
    /* button_state */ b32 kRight;
    /* button_state */ b32 kCtrl;
    /* button_state */ b32 kShift;
    /* button_state */ b32 kAlt;
    /* event_state  */ b32 KeyboardCharacterCame;
    c8 KeyboardCharacter;

    /* gamepad */
    //todo
    
    /* sound */
    i16 *Samples;
    i32  SamplesPerSecond;
    i32  SampleCount;

    /* memory */
    app_memory Memory;

    /* functions */
    platform_allocate_memory_callback      *AllocateMemoryCallback;
    platform_free_memory_callback          *FreeMemoryCallback;
    platform_load_file_callback            *LoadFileCallback;
    platform_free_file_callback            *FreeFileCallback;
    platform_load_file_to_arena_callback   *LoadFileToArenaCallback;
    platform_free_file_from_arena_callback *FreeFileFromArenaCallback;
    platform_write_file_callback           *WriteFileCallback;
    platform_report_error_callback         *ReportErrorCallback;
    platform_report_error_and_die_callback *ReportErrorAndDieCallback;
} platform;

#define APP_INIT(Name) void Name(platform *p)
typedef APP_INIT(app_init_callback);
        APP_INIT(AppInitStub) {};

#define APP_RELOAD(Name) void Name(platform *p)
typedef APP_RELOAD(app_reload_callback);
        APP_RELOAD(AppReloadStub) {};

#define APP_UPDATE(Name) void Name(platform *p)
typedef APP_UPDATE(app_update_callback);
        APP_UPDATE(AppUpdateStub) {};

#define APP_DEINIT(Name) void Name(platform *p)
typedef APP_DEINIT(app_deinit_callback);
        APP_DEINIT(AppDeinitStub) {};

#endif//PLATFORM_H
