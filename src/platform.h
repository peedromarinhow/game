#include "lingo.h"
#include "maths.h"

#ifndef PLATFORM_H
#define PLATFORM_H

#define KEYBOARD_MAX_BUTTONS 4
#define MOUSE_MAX_BUTTONS    3

typedef struct _button_state {
    i32 HalfTransitionCount;
    b32 EndedDown;
} button_state;

//note:
//  this is how the platform and the app communicate with each other.
typedef struct _platform {
    // metadata
    char *ExecutablePath;
    char *WorkingDirectoryPath;

    // options
    b32 Fullscreen;
    b32 Running;
    iv2 WindowSize;
    rv2 MousePos;
    i16 dMouseWheel;
    r32 dtForFrame;

    // input
    union {
        button_state Buttons[KEYBOARD_MAX_BUTTONS];
        struct {
            button_state Up;
            button_state Down;
            button_state Left;
            button_state Right;
        };
    } Keyboard;
    union {
        button_state Buttons[MOUSE_MAX_BUTTONS];
        struct {
            button_state Left;
            button_state Right;
            button_state Middle;
        };
    } Mouse;
    u64 CharacterInput;

    //todo: gamepad
    
    // sound
    i16 *Samples;
    i32 SamplesPerSecond;
    i32 SampleCount;

    //todo: memory

    //todo: functions
} platform;

#define APP_UPDATE(Name) void Name(platform *Platform)
typedef APP_UPDATE(app_update_callback);
APP_UPDATE(AppUpdateStub) {};

#endif