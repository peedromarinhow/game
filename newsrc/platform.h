#include "lingo.h"

#define KEYBOARD_MAX_BUTTONS 30
#define MOUSE_MAX_BUTTONS    3

typedef struct _button_state {
    i32 HalfTransitionCount;
    b32 EndedDown;
} button_state;

//note:
//  this is how the platform and the app communicate with
//  each other.
typedef struct _platform {
    // metadata
    char *ExecutableFolderPath;
    char *ExecutableAbsolutePath;
    char *WorkingDirectoryPath;

    // options
    b32 Fullscreen;
    r32 dtForTrame;
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
    i32  SamplesPerSecond;
    i32  SampleCount;
    i16 *Samples;

    // memory
    //todo: permanent and transient

    // functions
    //todo: debug file IO
} platform;

#define APP_UPDATE(Name) \
    void Name(platform *Platform)
typedef APP_UPDATE(app_update_callback);
