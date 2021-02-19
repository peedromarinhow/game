
#ifndef PLATFORM_H
#define PLATFORM_H

#include "lingo.h"
#include "maths.h"
#include "memory.h"

#define KEYBOARD_MAX_BUTTONS 4
#define MOUSE_MAX_BUTTONS    3

typedef struct _button_state {
    i32 HalfTransitionCount;
    b32 EndedDown;
} button_state;

//note:
//  this is how the platform and the app communicate with each other.
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

    //todo: gamepad
    
    /* sound */
    i16 *Samples;
    i32 SamplesPerSecond;
    i32 SampleCount;

    /* memory */
    app_memory Memory;

    //todo: functions
} platform;

#define APP_INIT(Name) void Name(platform *Plat)
typedef APP_INIT(app_init_callback);
APP_INIT(AppInitStub) {};

#define APP_UPDATE(Name) void Name(platform *Plat)
typedef APP_UPDATE(app_update_callback);
APP_UPDATE(AppUpdateStub) {};

#endif
