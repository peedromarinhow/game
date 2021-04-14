#include "lingo.h"
#include "api.h"

#include "maths.h"
#include "platform.h"
#include "memory.h"
#include "opengl.h"

// #include "graphics.h"
#include "renderer.h"
// #include "buffer.h"

// #include "ui.h"

typedef struct _app_state {
    // keymap *Keymap;  
    // buffer *Buffer;
    font    RobotoMono;
    font    Roboto;

    renderer Renderer;
} app_state;

external APP_INIT(Init) {
    Assert(sizeof(app_state) <= p->Memory.Size);
    app_state *State = (app_state *)p->Memory.Contents;

    platform_api Platform;

    Platform.AllocateMemory    = p->AllocateMemoryCallback;
    Platform.FreeMemory        = p->FreeMemoryCallback;
    Platform.LoadFile          = p->LoadFileCallback;
    Platform.FreeFile          = p->FreeFileCallback;
    Platform.LoadFileToArena   = p->LoadFileToArenaCallback;
    Platform.FreeFileFromArena = p->FreeFileFromArenaCallback;
    Platform.WriteFile         = p->WriteFileCallback;
    Platform.ReportError       = p->ReportErrorCallback;
    Platform.ReportErrorAndDie = p->ReportErrorAndDieCallback;

    // State->Keymap     = CreateMyKeymap();
    // State->Buffer     = CreateBuffer(2, "a.c");
    // State->RobotoMono = LoadFont(Platform, "roboto_mono.ttf", 400, 24);
    // State->Roboto     = LoadFont(Platform, "roboto.ttf", 400, 32);

    bcolor c;
    c.r = 0;
    c.g = 0;
    c.b = 0;
    c.a = 255;

    renderer Renderer = {
        .BufferSize = Kilobytes(2),
        .PushBuffer = Platform.AllocateMemory(Kilobytes(2)),
        .TargetDim  = p->WindowDimensions,
        .ClearColor = c
    };

    State->Renderer = Renderer;

    c.r = 255;
    c.g = 255;
    c.b = 255;
    c.a = 255;

    DrawRect(&State->Renderer, rectf_(10, 10, 30, 30), c);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    // Clear(p->WindowDimensions, HexToColor(0x20202000));
    app_state *State = (app_state *)p->Memory.Contents;

    // DrawRectPro(ORIGIN_CENTERED, rv2_(100, 100), rv2_(100, 100), HexToColor(0xFA6060FF), 0, (color){0});
    // DrawBuffer(rv2_(16, p->WindowDimensions.y - 32), State->Buffer, &State->RobotoMono, State->RobotoMono.Size);
    // c8 *Text = "aAJTiI\nabcdefg\n.........g";
    // rv2 Pos = rv2_(16, p->WindowDimensions.y - 64);
    // rv2 Dim = GetTextSize(&State->RobotoMono, Text, 0, 0, State->RobotoMono.Size, 0, 0);
    // DrawRect(ORIGIN_TOPLEFT, rv2_(Pos.x, Pos.y + State->RobotoMono.Size), Dim, HexToColor(0x4040FFFF));
    // DrawText(&State->RobotoMono, Text, Pos, 24, 0, 0, HexToColor(0xFA6060FF));
    
    // DrawTextBackGround(&State->Roboto, "THIS IS A BUTTON", rv2_(400, 400), 0, HexToColor(0xFAFAFAFF), HexToColor(0x404040FF));

    // key Key = KEY_NONE;
    
    // if (p->kDelete)
    //     Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_DEL);
    // if (p->kBack)
    //     Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_BACK);
    // if (p->kLeft)
    //     Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_LEFT);
    // if (p->kRight)
    //     Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_RIGHT);
    // if (p->kHome)
    //     Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_HOME);
    // if (p->kEnd)
    //     Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_END);
    // if (p->kReturn)
    //     Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, KEY_RETURN);
    // if (p->kChar)
    //     Key = GetKeyComb(p->kCtrl, p->kAlt, p->kShift, p->Char);
    
    // State->Keymap->Commands[Key].Func((command_context){State->Buffer, p->Char});

    Render(&State->Renderer);

/*
    render_group RenderGroup;

    Render(RenderGroup);
*/
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;

    // State->Keymap     = CreateMyKeymap();
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}

#if 0
#endif
