#include <windows.h>
#include <gl/gl.h>

#include "lingo.h"
#include "maths.h"
#include "platform.h"
#include "memory.h"

typedef struct _app_state {
    rv2 PlayerPos;
    memory_arena FreeTypeTestArena;
} app_state;

__declspec(dllexport) APP_INIT(Init) {
    Assert(sizeof(app_state) <= Plat->Memory.Size);
    app_state *State = (app_state *)Plat->Memory.Contents;
    State->PlayerPos = (rv2){100, 100};
}

__declspec(dllexport) APP_UPDATE(Update) {
    app_state *State = (app_state *)Plat->Memory.Contents;

    glViewport(0, 0, Plat->WindowSize.Width, Plat->WindowSize.Height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    r32 a = 2.0f/Plat->WindowSize.Width;
    r32 b = 2.0f/Plat->WindowSize.Height;
    r32 Proj[] = {
         a,  0,  0,  0,
         0, -b,  0,  0,
         0,  0,  1,  0,
        -1,  1,  0,  1
    };
    glLoadMatrixf(Proj);
}
