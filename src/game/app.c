#include "lingo.h"
#include "maths.h"
#include "platform.h"
#include "memory.h"

#include "app.h"

typedef struct _platform_api {
    platform_allocate_memory_callback      *AllocateMemory;
    platform_free_memory_callback          *FreeMemory;
    platform_load_file_callback            *LoadFile;
    platform_free_file_callback            *FreeFile;
    platform_load_file_to_arena_callback   *LoadFileToArena;
    platform_free_file_from_arena_callback *FreeFileFromArena;
    platform_write_file_callback           *WriteFile;
    platform_get_dir_filenames             *GetDirFilenames;
    platform_report_error_callback         *ReportError;
    platform_report_error_and_die_callback *ReportErrorAndDie;
} platform_api;

global platform_api GlobalPlatformApi;

internal void SetApi(platform *p) {
    GlobalPlatformApi.AllocateMemory    = p->AllocateMemoryCallback;
    GlobalPlatformApi.FreeMemory        = p->FreeMemoryCallback;
    GlobalPlatformApi.LoadFile          = p->LoadFileCallback;
    GlobalPlatformApi.FreeFile          = p->FreeFileCallback;
    GlobalPlatformApi.LoadFileToArena   = p->LoadFileToArenaCallback;
    GlobalPlatformApi.FreeFileFromArena = p->FreeFileFromArenaCallback;
    GlobalPlatformApi.WriteFile         = p->WriteFileCallback;
    GlobalPlatformApi.GetDirFilenames   = p->GetDirFilenames;
    GlobalPlatformApi.ReportError       = p->ReportErrorCallback;
    GlobalPlatformApi.ReportErrorAndDie = p->ReportErrorAndDieCallback;
}

typedef struct _app_state {
    rv2       PlayerPos;
    rv2       PlayerVel;
    renderer *Renderer;
} app_state;

//note:
// coordinates in engine range from 0-20 (x)
// and 0-10 (y)
inline rv2 EngineCoordToScreenCoord(rv2 Coord, rectf32 Screen) {
    return rv2_(Coord.x * Screen.w/200.0f, -Coord.y * Screen.h/100.0f + Screen.y + Screen.h/2.0f);
}

external APP_INIT(Init) {
    Assert(sizeof(app_state) <= p->Memory.Size);
    app_state *State = (app_state *)p->Memory.Contents;
    SetApi(p);

    LoadFont(State->Renderer, &GlobalPlatformApi, "roboto.ttf", 400, 24);
    State->PlayerPos = rv2_(100, 50);
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;
    SetApi(p);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    gBegin(rv2_(0, 0), p->WindowDim, HexToColor(0x000000FF));
    rectf32 Screen = {p->WindowDim.x/2, p->WindowDim.y/2.f,
                      p->WindowDim.x,   p->WindowDim.x/2.f};
    gDrawRectFromCenter(rv2_(Screen.x, Screen.y),
                        rv2_(Screen.w, Screen.h),
                        HexToColor(0x404550FF));
    
    const rv2 Gravity = rv2_(0, -50);

    if (p->kUp)
        State->PlayerVel.y += 10;
    if (p->kDown)
        State->PlayerVel.y -= 10;
    if (p->kRight)
        State->PlayerVel.x += 10;
    if (p->kLeft)
        State->PlayerVel.x -= 10;

    if (State->PlayerVel.x < 100 || State->PlayerVel.y < 100) {
        State->PlayerVel.x += p->dtForFrame * Gravity.x;
        State->PlayerVel.y += p->dtForFrame * Gravity.y;
    }

    rv2 NewPos = rv2_(State->PlayerPos.x + p->dtForFrame * State->PlayerVel.x,
                     State->PlayerPos.y + p->dtForFrame * State->PlayerVel.y);

    if (NewPos.y < 100 &&
        NewPos.y > 0   &&
        NewPos.x < 200 &&
        NewPos.x > 0)
    {
        State->PlayerPos = NewPos;
    }
    else {
        State->PlayerVel.x = 0;
        State->PlayerVel.y = 0;
    }
    
    rv2 ScaledPlayerVel = rv2_(State->PlayerPos.x + State->PlayerVel.x/10.f,
                               State->PlayerPos.y + State->PlayerVel.y/10.f);
    gDrawLineFromPoints(EngineCoordToScreenCoord(State->PlayerPos, Screen),
                        EngineCoordToScreenCoord(ScaledPlayerVel,  Screen),
                        1.f, HexToColor(0xFF2020FF));
    gDrawRectFromCenter(EngineCoordToScreenCoord(State->PlayerPos, Screen),
                        rv2_(10, 10), HexToColor(0x908585FF));
    
    c8 *Text = "public static void main()";
    rv2 TextPos = rv2_(10, 50);
    rv2 TextDim = rv2_(10, 10);
    gDrawText(State->RobotoMono, Text, EngineCoordToScreenCoord(TextPos, Screen),
              State->RobotoMono.Size, 0, -30, HexToColor(0xFFFFFFFF), &TextDim);
    
    rv2 TextRectPos = rv2_(0, 0);
    TextRectPos.w = EngineCoordToScreenCoord(TextPos, Screen).x + TextDim.w/2.f;
    TextRectPos.h = EngineCoordToScreenCoord(TextPos, Screen).y - TextDim.h/2.f;
    gDrawRectFromCenter(TextRectPos, TextDim, HexToColor(0xAA4045FF));
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}

#if 0
    if (p->WindowResized)
        State->Pos = rv2_(200, 200);

    const rv2 Gravity = rv2_(0, 1000);

    if (p->kUp)
        State->Vel.y = -1000;
if (p->kDown)
        State->Vel.y =  1000;
    if (p->kRight)
        State->Vel.x =  500;
    if (p->kLeft)
        State->Vel.x = -500;

    if (State->Vel.x < 1000 || State->Vel.y < 1000) {
        State->Vel.x += p->dtForFrame * Gravity.x;
        State->Vel.y += p->dtForFrame * Gravity.y;
    }

    rv2 NewPos = rv2_(State->Pos.x + p->dtForFrame * State->Vel.x,
                     State->Pos.y + p->dtForFrame * State->Vel.y);

    if (NewPos.y < Screen.h / 2 + Screen.y &&
        NewPos.y > Screen.y - Screen.h / 2 &&
        NewPos.x < Screen.w / 2 + Screen.x &&
        NewPos.x > Screen.x - Screen.w / 2)
    {
        State->Pos = NewPos;
    }
    else {
        State->Vel.x = 0;
        State->Vel.y = 0;
    }

    State->Size = rv2_(50, 50);
    State->Size.x += p->dMouseWheel/32;
    State->Size.y += p->dMouseWheel/32;

    gDrawRectFromCenter(State->Pos, State->Size, HexToColor(0.8f, 0.2f, 0.25f, 1));
    // color4f Color = HexToColor(1, 0, 0, 1);
    // if (p->MouseLeft)
    //     Color = HexToColor(1, 1, 0, 1);
    // if (p->MouseRight)
    //     Color = HexToColor(1, 0, 1, 1);

    // gDrawRectFromCenter(p->MousePos, rv2_(100, 100), Color);

    // texture a = State->Temp;
    // gDrawTexture(a, rv2_(p->WindowDim.w/2, p->WindowDim.h/2), rv2_(a.w, a.h));

    // State->AnimationTime    = 0;
    // State->AnimationRectPos = rv2_(p->WindowDim.w/2, p->WindowDim.h/2);

    // file Font = p->LoadFile(&Arena, "d:/code/platform-layer/data/im_fell_french_canon.ttf");
    // c8   TempBitmap[512*512];
    // stbtt_BakeFontBitmap(Font.Data, 0, 50.0, TempBitmap, 512, 512, 32, 96, CharacterData); // no guarantee this fits!
    // glGenTextures(1, &FontTexture);
    // glBindTexture(GL_TEXTURE_2D, FontTexture);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512,512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, TempBitmap);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // p->WriteFile_((void *)TempBitmap, 512*512, "d:/code/platform-layer/data/im_fell_french_canon.bmp");
    // p->FreeFile(&Arena, Font);

    // DawText(p->WindowDim.w/2, 100, "LOREM IPSVM");
    // if (State->AnimationTime <= 2) {
    //     State->AnimationTime      += p->dtForFrame;
    //     State->AnimationRectPos.y -= f(State->AnimationTime, 2, 300);
    // }
    // else {
    //     State->AnimationTime += 0;
    // }
    
    // gDrawRectFromCenter(State->AnimationRectPos, rv2_(100, 100), Color);
    // file Bitmap = p->LoadFile(&Arena, "D:/code/platform-layer/data/map.bmp");
    // bitmap_header *Header = (bitmap_header *)Bitmap.Data;
    // State->Image.w      = Header->Width;
    // State->Image.h      = Header->Height;
    // State->Image.Pixels = (u32 *)((u8 *)Bitmap.Data + Header->BitmapOffset);;
    // glGenTextures(1, &State->Image.Handle);

    glViewport(0, 0, p->WindowDim.w, p->WindowDim.h);
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    r32 a = 2.0f/p->WindowDim.w;
    r32 b = 2.0f/p->WindowDim.h;
    r32 Proj[] = {
         a,  0,  0,  0,
         0, -b,  0,  0,
         0,  0,  1,  0,
        -1,  1,  0,  1
    };
    glLoadMatrixf(Proj);
    DrawTexture(State->Image);

    // glBindTexture(GL_TEXTURE_2D, State->Image.Handle);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, State->Image.w, State->Image.h, 0,
    //              GL_BGR_EXT, GL_UNSIGNED_BYTE, State->Image.Pixels);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    // glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    // glEnable(GL_TEXTURE_2D);
    // glMatrixMode(GL_TEXTURE);
    // glLoadIdentity();
    // glBegin(GL_TRIANGLES);
    //     r32 P = 1.0f;
    //     // lower tri
    //     glTexCoord2f(0.0f, 0.0f);
    //     glVertex2f(-P,-P);
    //     glTexCoord2f(1.0f, 0.0f);
    //     glVertex2f(P, -P);
    //     glTexCoord2f(1.0f, 1.0f);
    //     glVertex2f(P, P);

    //     // higher tri
    //     glTexCoord2f(0.0f, 0.0f);
    //     glVertex2f(-P, -P);
    //     glTexCoord2f(1.0f, 1.0f);
    //     glVertex2f(P,P);
    //     glTexCoord2f(0.0f, 1.0f);
    //     glVertex2f(-P, P);
    // glEnd();

    glViewport(0, 0, p->WindowDim.w, p->WindowDim.h);

    glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();

    // glMatrixMode(GL_PROJECTION);
    // glLoadIdentity();

    // r32 a = 2.0f/p->WindowDim.w;
    // r32 b = 2.0f/p->WindowDim.h;
    // r32 Proj[] = {
    //      a,  0,  0,  0,
    //      0, -b,  0,  0,
    //      0,  0,  1,  0,
    //     -1,  1,  0,  1
    // };
    // glLoadMatrixf(Proj);

    // glBindTexture(GL_TEXTURE_2D, State->Image.Handle);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, State->Image.w, State->Image.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)State->Image.Pixels);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // glEnable(GL_TEXTURE_2D);
    // glBegin(GL_QUADS);
    //     glVertex2i(100, 100);
    //     glVertex2i(100, 500);
    //     glVertex2i(500, 500);
    //     glVertex2i(500, 100);
    // glEnd();

    glBindTexture(GL_TEXTURE_2D, State->Image.Handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, State->Image.w, State->Image.h, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, State->Image.Pixels);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        glVertex2f(0, 0);
        glVertex2f(p->WindowDim.w, 0);
        glVertex2f(p->WindowDim.w, p->WindowDim.h);
        glVertex2f(0, p->WindowDim.h);
    } glEnd();
    // DrawRectangleFromCenter((rv2){p->WindowDim.w/2.0f, p->WindowDim.h/2.0f}, p->Mouse.Pos);

    // glViewport(0, 0, p->WindowDim.w, p->WindowDim.h);

    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();

    // glMatrixMode(GL_PROJECTION);
    // glLoadIdentity();

    // r32 a = 2.0f/p->WindowDim.w;
    // r32 b = 2.0f/p->WindowDim.h;
    // r32 Proj[] = {
    //      a,  0,  0,  0,
    //      0, -b,  0,  0,
    //      0,  0,  1,  0,
    //     -1,  1,  0,  1
    // };
    // glLoadMatrixf(Proj);

    // DrawRectangleFromCenter((rv2){p->WindowDim.w/2.0f, p->WindowDim.h/2.0f}, p->Mouse.Pos);
    // DrawTexture(State->Map, (rv2){p->WindowDim.w/2.0f, p->WindowDim.h/2.0f}, (rv2){p->WindowDim.w, p->WindowDim.h});
    // DrawRectangleStrokeFromCenter((rv2){p->WindowDim.w/2.0f, p->WindowDim.h/2.0f}, p->Mouse.Pos, 10);
    // DrawFilledCircle((rv2){100, 100}, 100, 100);

    // glViewport(0, 0, p->WindowDim.Width, p->WindowDim.Height);

    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();

    // glMatrixMode(GL_PROJECTION);
    // glLoadIdentity();

    // r32 a = 2.0f/p->WindowDim.Width;
    // r32 b = 2.0f/p->WindowDim.Height;
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
    //     if (p->Mouse.Moved.EndedHappening && p->Keyboard.Alt.EndedDown) {
    //         glColor3f(1, 0, 0);
    //     }
    //     glVertex2f(State->PlayerPos.x, State->PlayerPos.y + State->MouseWheel);
    //     glVertex2f(200, 200);
    // } glEnd();

    // State->PlayerPos = p->Mouse.Pos;
    // State->MouseWheel += p->Mouse.dWheel / 6.0f;
    glViewport(0, 0, p->WindowDim.Width, p->WindowDim.Height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    r32 a = 2.0f/p->WindowDim.x;
    r32 b = 2.0f/p->WindowDim.y;
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
            (p->Mouse.Pos.x),
            (p->Mouse.Pos.y)
        };

        rv2 UpperRightCorner = {
            (p->Mouse.Pos.x + 100.0f),
            (p->Mouse.Pos.y + 100.0f)
        };

        glVertex2f(LowerLeftCorner.x, LowerLeftCorner.y);
        glVertex2f(UpperRightCorner.x, UpperRightCorner.y);

    } glEnd();

    if (p->Mouse.Left.EndedDown  ||
        p->Mouse.Right.EndedDown ||
        p->Mouse.Middle.EndedDown)
    {
        glBegin(GL_TRIANGLES); {

            glColor3f(1.0f, 1.0f, 1.0f);

            rv2 P1 = {100, 100};
            rv2 P2 = {200, 100};
            rv2 P3 = {100, 200};

            if ((100 <= p->Mouse.Pos.x && p->Mouse.Pos.x <= 200) &&
                (100 <= p->Mouse.Pos.y && p->Mouse.Pos.y <= 200) &&
                p->Mouse.Left.EndedDown)
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
        if (p->Keyboard.Character == 'a')
            glVertex2f(P2.x, P2.y);

    } glEnd();

    a = p->WindowDim.x;
    b = p->WindowDim.y;
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
        c32 w = ExpITheta((p->Mouse.Pos.x/(r32)a)*4.0f*PI32);
        glVertex2f(Origin.a, Origin.b);
        glVertex2f(w.a, w.b);
    } glEnd();
#endif
