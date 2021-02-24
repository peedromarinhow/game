#include <windows.h>
#include <gl/gl.h>

#include "lingo.h"
#include "maths.h"
#include "platform.h"
#include "memory.h"

typedef struct _app_state {
    rv2 PlayerPos;
    memory_arena Arena;
} app_state;

__declspec(dllexport) APP_INIT(Init) {
    Assert(sizeof(app_state) <= Plat->Memory.Size);
    app_state *State = (app_state *)Plat->Memory.Contents;

    State->PlayerPos = (rv2){100, 100};
    State->Arena = InitializeArena(Megabytes(4), ((u8 *)Plat->Memory.Contents + sizeof(app_state)));
}

__declspec(dllexport) APP_UPDATE(Update) {
    app_state *State = (app_state *)Plat->Memory.Contents;

    if (Plat->Keyboard.Down.EndedDown) {
        Plat->ReportError("TEST", "no error, just a test...");
    }

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

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(10.0f);
    glBegin(GL_LINES); {
        glColor3f(1, 1, 1);
        if (Plat->Mouse.Moved.EndedHappening && Plat->Keyboard.Alt.EndedDown) {
            glColor3f(1, 0, 0);
        }
        glVertex2f(State->PlayerPos.x, State->PlayerPos.y);
        glVertex2f(200, 200);
    } glEnd();

    State->PlayerPos = Plat->Mouse.Pos;
}

__declspec(dllexport) APP_DEINIT(Deinit) {
    app_state *State = (app_state *)Plat->Memory.Contents;
    file ThisFile = Plat->LoadFile(&State->Arena, __FILE__);
    Plat->WriteFile(ThisFile.Data, ThisFile.Size, "a.c");
}
