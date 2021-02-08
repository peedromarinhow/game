#include "lingo.h"
#include "maths.h"

#ifndef PLATFORM_H
#define PLATFORM_H

#define KEYBOARD_MAX_BUTTONS 30
#define MOUSE_MAX_BUTTONS    3

typedef struct _button_state {
    i32 HalfTransitionCount;
    b32 EndedDown;
} button_state;

//note:
//  this is how the platform and the app communicate with each other.
typedef struct _platform {
    // metadata
    char ExecutablePath      [MAX_PATH];
    char WorkingDirectoryPath[MAX_PATH];

    // options
    b32 Fullscreen;
    b32 Running;
    iv2 WindowSize;
    rv2 MousePos;
        //todo: mouse wheel
    r32 dtForFrame;
        //todo: window dimensions, etc  

    // input
    button_state KeyboardButtons[KEYBOARD_MAX_BUTTONS];
    button_state MouseButtons[MOUSE_MAX_BUTTONS];
    i32 MouseX;
    i32 MouseY;
    i32 MouseZ;
        //todo: mouse X and Y as vectors
        //todo: gamepad
    
    // sound
    //todo

    // memory
    //todo

    // functions
    //todo
} platform;

#define APP_UPDATE(Name) void Name(platform *Platform)
typedef APP_UPDATE(app_update_callback);
APP_UPDATE(AppUpdateStub)

#endif