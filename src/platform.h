
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

#define PLATFORM_FREE_FILE(Name) void Name(memory_arena *Arena, file File)
typedef PLATFORM_FREE_FILE(platform_free_file_callback);

#define PLATFORM_LOAD_FILE(Name) file Name(memory_arena *Arena, char *Filename)
typedef PLATFORM_LOAD_FILE(platform_load_file_callback);

#define PLATFORM_WRITE_FILE(Name) void Name(void *Data, u64 Size, char *Filename)
typedef PLATFORM_WRITE_FILE(platform_write_file_callback);

#define PLATFORM_REPORT_ERROR(Name) void Name(char *Title, char *ErrorMessage)
typedef PLATFORM_REPORT_ERROR(platform_report_error_callback);

#define PLATFORM_REPORT_ERROR_AND_DIE(Name) void Name(char *Title, char *ErrorMessage)
typedef PLATFORM_REPORT_ERROR_AND_DIE(platform_report_error_and_die_callback);

typedef struct _button_state {
    i32 HalfTransitionCount;
    b32 EndedDown;
} button_state;

//note: this is how the platform and the app communicate with each other.
#define KEYBOARD_MAX_BUTTONS 4
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

    /* mouse input */
    i16 dMouseWheel;
    rv2 MousePos;
    union {
        button_state MouseButtons[MOUSE_MAX_BUTTONS];
        struct {
            button_state MouseLeft;
            button_state MouseRight;
            button_state MouseMiddle;
        };
    };
    //note: structure this? ie: Mouse.Buttons.Left or whatever?

    /* keyboard input */
    union {
        button_state KeybardButtons[KEYBOARD_MAX_BUTTONS];
        struct {
            button_state KeyboardUp;
            button_state KeyboardDown;
            button_state KeyboardLeft;
            button_state KeyboardRight;
        };
    };
    u64 CharacterInput;
    //note: structure this? ie: Keboard.Buttons.Left or whatever?

    /* gamepad */
    //todo
    
    /* sound */
    i16 *Samples;
    i32 SamplesPerSecond;
    i32 SampleCount;

    /* memory */
    app_memory Memory;

    /* functions */
    platform_free_file_callback            *FreeFile;
    platform_load_file_callback            *LoadFile;
    platform_write_file_callback           *WriteFile;
    platform_report_error_callback         *ReportError;
    platform_report_error_and_die_callback *ReportErrorAndDie;
} platform;

#define APP_INIT(Name) void Name(platform *Plat)
typedef APP_INIT(app_init_callback);
        APP_INIT(AppInitStub) {};

#define APP_UPDATE(Name) void Name(platform *Plat)
typedef APP_UPDATE(app_update_callback);
        APP_UPDATE(AppUpdateStub) {};

#endif
