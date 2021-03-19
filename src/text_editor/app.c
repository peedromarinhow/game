#include "lingo.h"
#include "maths.h"
#include "platform.h"
#include "memory.h"
#include "opengl.h"
#include "app.h"

#include "graphics.h"
#include "fonts.h"

typedef struct _app_state {
    font RobotoMono;
} app_state;

external APP_INIT(Init) {
    Assert(sizeof(app_state) <= p->Memory.Size);
    app_state *State = (app_state *)p->Memory.Contents;

    AllocateMemory    = p->AllocateMemoryCallback;
    FreeMemory        = p->FreeMemoryCallback;
    LoadFile          = p->LoadFileCallback;
    FreeFile          = p->FreeFileCallback;
    LoadFileToArena   = p->LoadFileToArenaCallback;
    FreeFileFromArena = p->FreeFileFromArenaCallback;
    WriteFile_        = p->WriteFileCallback;
    ReportError       = p->ReportErrorCallback;
    ReportErrorAndDie = p->ReportErrorAndDieCallback;

    State->RobotoMono = LoadFont("roboto_mono.ttf", 400, 50);
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;

    AllocateMemory    = p->AllocateMemoryCallback;
    FreeMemory        = p->FreeMemoryCallback;
    LoadFile          = p->LoadFileCallback;
    FreeFile          = p->FreeFileCallback;
    LoadFileToArena   = p->LoadFileToArenaCallback;
    FreeFileFromArena = p->FreeFileFromArenaCallback;
    WriteFile_        = p->WriteFileCallback;
    ReportError       = p->ReportErrorCallback;
    ReportErrorAndDie = p->ReportErrorAndDieCallback;
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;
    gBegin(Rv2(0, 0), p->WindowDimensions, Color4f(0.1f, 0.2f, 0.25f, 1));
    
    c8 *Text = "public static void main()";
    rv2 TextPos = Rv2(100, 500);
    rv2 TextDim = Rv2(100, 100);
    gDrawText(State->RobotoMono, Text, TextPos,
              State->RobotoMono.Size, 0, -30,
              Color4f(1, 1, 1, 1), &TextDim);

    gDrawRectFromCenter(TextPos, TextDim, Color4f(0.8f, 0.3f, 0.4f, 0.5));
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}
