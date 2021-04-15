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

void DrawRectPro(rv2 Pos, rv2 Size, color Color,
                 r32 StrokeWidth, color StrokeColor)
{
    rv2 TopLeft     = rv2_(0, 0);
    rv2 TopRight    = rv2_(0, 0);
    rv2 BottomRight = rv2_(0, 0);
    rv2 BottomLeft  = rv2_(0, 0);

    TopLeft     = rv2_(Pos.x, Pos.y + Size.h);
    TopRight    = rv2_(Pos.x + Size.w, Pos.y + Size.h);
    BottomRight = rv2_(Pos.x + Size.w, Pos.y);
    BottomLeft  = rv2_(Pos.x, Pos.y);

    if (Color.a != (0/255.f)) {
        glBegin(GL_POLYGON); {
            glColor4f(Color.r, Color.g, Color.b, Color.a);
            glVertex2f(TopLeft.x, TopLeft.y);
            glVertex2f(TopRight.x, TopRight.y);
            glVertex2f(BottomRight.x, BottomRight.y);
            glVertex2f(BottomLeft.x, BottomLeft.y);
        } glEnd();
    }

    if (StrokeWidth > 0) {
        glLineWidth(StrokeWidth);
        glBegin(GL_LINE_LOOP); {
            glColor4f(StrokeColor.r, StrokeColor.g, StrokeColor.b, StrokeColor.a);
            glVertex2f(TopLeft.x, TopLeft.y);
            glVertex2f(TopRight.x, TopRight.y);
            glVertex2f(BottomRight.x, BottomRight.y);
            glVertex2f(BottomLeft.x, BottomLeft.y);
        } glEnd();
    }
}

typedef struct _app_state {
    // keymap *Keymap;  
    // buffer *Buffer;
    // font    RobotoMono;
    // font    Roboto;

    u16 RobotoMono;

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

    // State->Keymap     = CreateMyKeymap();
    // State->Buffer     = CreateBuffer(2, "a.c");
    State->RobotoMono = 0;
    // State->Roboto     = LoadFont(Platform, "roboto.ttf", 400, 32);
    State->Renderer.Fonts[State->RobotoMono] = LoadFont(&PlatformApi, "roboto_mono.ttf", 400, 24);

    colorb c;

    c.r = 225;
    c.g = 225;
    c.b = 225;
    c.a = 255;

    DrawRect(&State->Renderer, rectf_(100, 100, 30, 30), c);
    DrawRect(&State->Renderer, rectf_(200, 100, 30, 30), c);
    DrawText(&State->Renderer, State->RobotoMono, rv2_(500, 300), "hello\nworld", State->Renderer.Fonts[State->RobotoMono].Height, 0, 0, c);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;
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
    Render(&State->Renderer, p->WindowDim, (colorb){0x202020FF});
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
