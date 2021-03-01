#include "graphics.h"
#include "lingo.h"
#include "maths.h"
#include "platform.h"
#include "memory.h"

typedef struct _app_state {
    memory_arena Arena;
    texture      Map;
} app_state;

__declspec(dllexport) APP_INIT(Init) {
    Assert(sizeof(app_state) <= Plat->Memory.Size);
    app_state *State = (app_state *)Plat->Memory.Contents;
    State->Arena = InitializeArena(Megabytes(4), ((u8 *)Plat->Memory.Contents + sizeof(app_state)));

    bitmap  Image = LoadBMP(&State->Arena, Plat->LoadFile, "map.bmp");
    State->Map    = GenTextureFromBitmap(Image);
}

__declspec(dllexport) APP_UPDATE(Update) {
    app_state *State = (app_state *)Plat->Memory.Contents;

    glViewport(0, 0, Plat->WindowSize.w, Plat->WindowSize.h);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    r32 a = 2.0f/Plat->WindowSize.w;
    r32 b = 2.0f/Plat->WindowSize.h;
    r32 Proj[] = {
         a,  0,  0,  0,
         0, -b,  0,  0,
         0,  0,  1,  0,
        -1,  1,  0,  1
    };
    glLoadMatrixf(Proj);

    // DrawRectangleFromCenter((rv2){Plat->WindowSize.w/2.0f, Plat->WindowSize.h/2.0f}, Plat->Mouse.Pos);
    DrawTexture(State->Map, (rv2){Plat->WindowSize.w/2.0f, Plat->WindowSize.h/2.0f}, (rv2){State->Map.w, State->Map.h});
    // DrawRectangleStrokeFromCenter((rv2){Plat->WindowSize.w/2.0f, Plat->WindowSize.h/2.0f}, Plat->Mouse.Pos, 10);
    // DrawFilledCircle((rv2){100, 100}, 100, 100);

}

__declspec(dllexport) APP_DEINIT(Deinit) {
    app_state *State = (app_state *)Plat->Memory.Contents;
}

#if 0
    // glViewport(0, 0, Plat->WindowSize.Width, Plat->WindowSize.Height);

    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();

    // glMatrixMode(GL_PROJECTION);
    // glLoadIdentity();

    // r32 a = 2.0f/Plat->WindowSize.Width;
    // r32 b = 2.0f/Plat->WindowSize.Height;
    // r32 Proj[] = {
    //      a,  0,  0,  0,
    //      0, -b,  0,  0,
    //      0,  0,  1,  0,
    //     -1,  1,  0,  1
    // };
    // glLoadMatrixf(Proj);

    // glEnable(GL_LINE_SMOOTH);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glLineWidth(10.0f);
    // glBegin(GL_LINES); {
    //     glColor3f(1, 1, 1);
    //     if (Plat->Mouse.Moved.EndedHappening && Plat->Keyboard.Alt.EndedDown) {
    //         glColor3f(1, 0, 0);
    //     }
    //     glVertex2f(State->PlayerPos.x, State->PlayerPos.y + State->MouseWheel);
    //     glVertex2f(200, 200);
    // } glEnd();

    // State->PlayerPos = Plat->Mouse.Pos;
    // State->MouseWheel += Plat->Mouse.dWheel / 6.0f;
    glViewport(0, 0, Plat->WindowSize.Width, Plat->WindowSize.Height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    r32 a = 2.0f/Plat->WindowSize.x;
    r32 b = 2.0f/Plat->WindowSize.y;
    r32 Proj[] = {
         a,  0,  0,  0,
         0, -b,  0,  0,
         0,  0,  1,  0,
        -1,  1,  0,  1
    };
    glLoadMatrixf(Proj);

    glBegin(GL_LINES); {

        glColor3f(1.0f, 1.0f, 1.0f);

        rv2 LowerLeftCorner = {
            (Plat->Mouse.Pos.x),
            (Plat->Mouse.Pos.y)
        };

        rv2 UpperRightCorner = {
            (Plat->Mouse.Pos.x + 100.0f),
            (Plat->Mouse.Pos.y + 100.0f)
        };

        glVertex2f(LowerLeftCorner.x, LowerLeftCorner.y);
        glVertex2f(UpperRightCorner.x, UpperRightCorner.y);

    } glEnd();

    if (Plat->Mouse.Left.EndedDown  ||
        Plat->Mouse.Right.EndedDown ||
        Plat->Mouse.Middle.EndedDown)
    {
        glBegin(GL_TRIANGLES); {

            glColor3f(1.0f, 1.0f, 1.0f);

            rv2 P1 = {100, 100};
            rv2 P2 = {200, 100};
            rv2 P3 = {100, 200};

            if ((100 <= Plat->Mouse.Pos.x && Plat->Mouse.Pos.x <= 200) &&
                (100 <= Plat->Mouse.Pos.y && Plat->Mouse.Pos.y <= 200) &&
                Plat->Mouse.Left.EndedDown)
            {                
                glColor3f(1.0f, 0.0f, 1.0f);
            }

            glVertex2f(P1.x, P1.y);
            glVertex2f(P2.x, P2.y);
            glVertex2f(P3.x, P3.y);

        } glEnd();
    }

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_LINES); {

        glColor3f(1.0f, 1.0f, 1.0f);

        rv2 P1 = {200, 200};
        rv2 P2 = {300, 300};

        glVertex2f(P1.x, P1.y);
        if (Plat->Keyboard.Character == 'a')
            glVertex2f(P2.x, P2.y);

    } glEnd();

    a = Plat->WindowSize.x;
    b = Plat->WindowSize.y;
    r32 ComplexProj[] = {
      b/a,   0,  0,  0,
        0,   1,  0,  0,
        0,   0,  1,  0,
        0,   0,  0,  1
    };
    glLoadMatrixf(ComplexProj);

    glBegin(GL_LINES); {
        glVertex2f(0,  1000);
        glVertex2f(0, -1000);

        glVertex2f( 1000, 0);
        glVertex2f(-1000, 0);

        c32 Origin = (c32){0, 0};
        c32 w = ExpITheta((Plat->Mouse.Pos.x/(r32)a)*4.0f*PI32);
        glVertex2f(Origin.a, Origin.b);
        glVertex2f(w.a, w.b);
    } glEnd();
#endif