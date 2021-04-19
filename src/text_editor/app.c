#include "lingo.h"
#include "api.h"

#include "maths.h"
#include "platform.h"
#include "memory.h"
#include "opengl.h"

platform_api GlobalPlatformApi;

#include "renderer.h"
#include "text_buffer.h"

typedef struct _app_state {
    keymap *Keymap;
    command_context CommandContext;

    id Roboto;
    id RobotoMono;

    renderer Renderer;
} app_state;

external APP_INIT(Init) {
    Assert(sizeof(app_state) <= p->Memory.Size);
    app_state *State = (app_state *)p->Memory.Contents;

    platform_api PlatformApi;

    PlatformApi.AllocateMemory    = p->AllocateMemoryCallback;
    PlatformApi.FreeMemory        = p->FreeMemoryCallback;
    PlatformApi.LoadFile          = p->LoadFileCallback;
    PlatformApi.FreeFile          = p->FreeFileCallback;
    PlatformApi.LoadFileToArena   = p->LoadFileToArenaCallback;
    PlatformApi.FreeFileFromArena = p->FreeFileFromArenaCallback;
    PlatformApi.WriteFile         = p->WriteFileCallback;
    PlatformApi.ReportError       = p->ReportErrorCallback;
    PlatformApi.ReportErrorAndDie = p->ReportErrorAndDieCallback;

    GlobalPlatformApi = PlatformApi;

    State->Keymap = CreateMyKeymap();

    State->CommandContext.Buffers[0] = CreateBuffer(8, "a.c");
    State->CommandContext.Buffers[1] = CreateBuffer(8, "b.c");
    State->CommandContext.GoalColumn = -1;

    State->RobotoMono = LoadFont(&State->Renderer, &PlatformApi, "roboto_mono.ttf", 400, 24);
    State->Roboto     = LoadFont(&State->Renderer, &PlatformApi, "roboto.ttf",      400, 32);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    key Key = KEY_NONE;
    
    if (p->kDelete)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_DEL);
    if (p->kBack)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_BACK);
    if (p->kTab)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_TAB);
    if (p->kLeft)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_LEFT);
    if (p->kRight)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_RIGHT);
    if (p->kUp)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_UP);
    if (p->kDown)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_DOWN);
    if (p->kHome)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_HOME);
    if (p->kPgUp)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_PG_UP);
    if (p->kPgDown)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_PG_DOWN);
    if (p->kEnd)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_END);
    if (p->kReturn)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_RETURN);
    if (p->kChar)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, p->Char);

    State->CommandContext.NoBuffers = 2;
    State->CommandContext.LastChar = p->Char;

    colorb c;

    c.r = 225;
    c.g = 225;
    c.b = 225;
    c.a = 255;

    DrawBuffer(&State->Renderer, State->CommandContext.Buffers[0], State->Roboto,
                rv2_(16, p->WindowDim.y - 32), State->Renderer.Fonts[State->Roboto].Height, 0, 0, c,
                p->mPos, p->mLeft);
    DrawBuffer(&State->Renderer, State->CommandContext.Buffers[1], State->Roboto,
                rv2_(p->WindowDim.x/2 + 16, p->WindowDim.y - 32), State->Renderer.Fonts[State->Roboto].Height, 0, 0, c,
                p->mPos, p->mLeft);
    
    State->Keymap->Commands[Key].Func(&State->CommandContext);

    Render(&State->Renderer, p->WindowDim, (colorb){0x202020FF});
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;

    GlobalPlatformApi.AllocateMemory    = p->AllocateMemoryCallback;
    GlobalPlatformApi.FreeMemory        = p->FreeMemoryCallback;
    GlobalPlatformApi.LoadFile          = p->LoadFileCallback;
    GlobalPlatformApi.FreeFile          = p->FreeFileCallback;
    GlobalPlatformApi.LoadFileToArena   = p->LoadFileToArenaCallback;
    GlobalPlatformApi.FreeFileFromArena = p->FreeFileFromArenaCallback;
    GlobalPlatformApi.WriteFile         = p->WriteFileCallback;
    GlobalPlatformApi.ReportError       = p->ReportErrorCallback;
    GlobalPlatformApi.ReportErrorAndDie = p->ReportErrorAndDieCallback;

    State->Keymap = CreateMyKeymap();
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}

#if 0
#endif
