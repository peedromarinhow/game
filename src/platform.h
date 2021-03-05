
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

#define PLATFORM_MEM_ALLOC(Name) void *Name(u32 Size)
typedef PLATFORM_MEM_ALLOC(platform_mem_alloc_callback);

#define PLATFORM_MEM_FREE(Name) void Name(void *Data)
typedef PLATFORM_MEM_FREE(platform_mem_free_callback);

#define PLATFORM_FILE_LOAD_ARENA(Name) file Name(memory_arena *Arena, char *Filename)
typedef PLATFORM_FILE_LOAD_ARENA(platform_load_file_arena_callback);

#define PLATFORM_FILE_FREE_ARENA(Name) void Name(memory_arena *Arena, file File)
typedef PLATFORM_FILE_FREE_ARENA(platform_file_free_arena_callback);

#define PLATFORM_FILE_LOAD(Name) file Name(char *Filename)
typedef PLATFORM_FILE_LOAD(platform_file_load_callback);

#define PLATFORM_FILE_FREE(Name) void Name(file File)
typedef PLATFORM_FILE_FREE(platform_file_free_callback);

#define PLATFORM_FILE_WRITE(Name) void Name(void *Data, u64 Size, char *Filename)
typedef PLATFORM_FILE_WRITE(platform_file_write_callback);

#define PLATFORM_REPORT_ERROR(Name) void Name(char *Title, char *ErrorMessage)
typedef PLATFORM_REPORT_ERROR(platform_report_error_callback);

#define PLATFORM_REPORT_ERROR_AND_DIE(Name) void Name(char *Title, char *ErrorMessage)
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
    char *ExecutablePath;
    char *WorkingDirectoryPath;

    /* options */
    b32 Fullscreen;
    b32 Running;
    iv2 WindowSize;
    r32 dtForFrame;
    b32 CtrlKeyWasDown;
    b32 ShiftKeyWasDown;
    b32 AltKeyWasDown;

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
    u64          KeyboardCharacter;

    /* gamepad */
    //todo
    
    /* sound */
    i16 *Samples;
    i32  SamplesPerSecond;
    i32  SampleCount;

    /* memory */
    app_memory Memory;

    /* functions */
    platform_mem_alloc_callback            *MemAllocCallback;
    platform_mem_free_callback             *MemFreeCallback;
    platform_file_load_callback            *FileLoadCallback;
    platform_file_free_callback            *FileFreeCallback;
    platform_load_file_arena_callback      *FileLoadArenaCallback;
    platform_file_free_arena_callback      *FileFreeArenaCallback;
    platform_file_write_callback           *FileWriteCallback;
    platform_report_error_callback         *ReportErrorCallback;
    platform_report_error_and_die_callback *ReportErrorAndDieCallback;
} platform;

#define APP_INIT(Name) void Name(platform *p)
typedef APP_INIT(app_init_callback);
        APP_INIT(AppInitStub) {};

#define APP_UPDATE(Name) void Name(platform *p)
typedef APP_UPDATE(app_update_callback);
        APP_UPDATE(AppUpdateStub) {};

#define APP_DEINIT(Name) void Name(platform *p)
typedef APP_DEINIT(app_deinit_callback);
        APP_DEINIT(AppDeinitStub) {};

#endif
