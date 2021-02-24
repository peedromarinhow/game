
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
    struct _Mouse {
        event_state Moved;
        i16 dWheel;
        rv2 Pos;
        union {
            button_state Buttons[MOUSE_MAX_BUTTONS];
            struct {
                button_state Left;
                button_state Right;
                button_state Middle;
            };
        };
    } Mouse;

    /* keyboard input */
    struct _Keyboard {
        union {
            button_state Buttons[KEYBOARD_MAX_BUTTONS];
            struct {
                button_state Up;
                button_state Down;
                button_state Left;
                button_state Right;
                button_state Ctrl;
                button_state Shift;
                button_state Alt;
            };
        };
        u64 Character;
    } Keyboard;

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

#define APP_DEINIT(Name) void Name(platform *Plat)
typedef APP_DEINIT(app_deinit_callback);
        APP_DEINIT(AppDeinitStub) {};

#endif
