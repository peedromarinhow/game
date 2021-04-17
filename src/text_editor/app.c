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
    buffer *Buffers[2];
    id CurrentBuffer;

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
    State->Buffers[0] = CreateBuffer(2, "a.c");
    State->Buffers[1] = CreateBuffer(2, "b.c");

    State->RobotoMono = LoadFont(&State->Renderer, &PlatformApi, "roboto_mono.ttf", 400, 24);
    State->Roboto     = LoadFont(&State->Renderer, &PlatformApi, "roboto.ttf",      400, 32);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;
    DrawBuffer(&State->Renderer, rv2_(16, p->WindowDim.y - 32), State->Buffers[State->CurrentBuffer]);
    DrawBuffer(&State->Renderer, rv2_(p->WindowDim.x + 16, p->WindowDim.y - 32), State->Buffers[State->CurrentBuffer]);

    key Key = KEY_NONE;
    
    if (p->kDelete)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_DEL);
    if (p->kBack)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_BACK);
    if (p->kLeft)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_LEFT);
    if (p->kRight)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_RIGHT);
    if (p->kHome)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_HOME);
    if (p->kEnd)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_END);
    if (p->kReturn)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_RETURN);
    if (p->kChar)
        Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, p->Char);
    
    State->Keymap->Commands[Key].Func((command_context){State->Buffers[State->CurrentBuffer], p->Char, State->CurrentBuffer, 2});

    colorb c;

    c.r = 225;
    c.g = 225;
    c.b = 225;
    c.a = 255;

    colorb d;

    d.r = p->mPos.x / 5;
    d.g = p->mPos.y / 5;
    d.b = p->mPos.y / 5 + p->mPos.x / 5;
    d.a = 255;

    c8   *Text = "Lorem ipsum\nDolor sit amet";
    font *Font = &State->Renderer.Fonts[State->Roboto];

    rv2 TextDim = MeasureText(Font, Text, Font->Height, 0, 0);

    rect r1 = rect_(100, 100, 30, 30);
    rect r2 = rect_(p->mPos.x, p->mPos.y, 30, 30);
    rect r3 = rect_(500, 500, GetVecComps(TextDim));

    DrawRect(&State->Renderer, r3, (colorb){0xFF909090});
    r3.y += Font->Height - Font->Ascender;
    DrawText(&State->Renderer, State->Roboto, r3.Pos, Text, Font->Height, 0, 0, d);
    
    DrawRect(&State->Renderer, r1, c);
    DrawRect(&State->Renderer, r2, c);
    DrawText(&State->Renderer, State->RobotoMono, rv2_(90,90), "hello\nworld", State->Renderer.Fonts[State->RobotoMono].Height, 0, 0, c);
    DrawText(&State->Renderer, State->Roboto,   rv2_(600,600), "hello\nworld", State->Renderer.Fonts[State->Roboto].Height,     0, 0, c);

    DrawGlyph(&State->Renderer, State->Roboto, 'W', p->mPos, c);

    if (AreRectsClipping(r1, r2))
        DrawRect(&State->Renderer, State->Renderer.TargetClipRect, c);

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
