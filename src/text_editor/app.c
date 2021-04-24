#include "lingo.h"
#include "platform.h"
#include "maths.h"
#include "api.h"

platform_api GlobalPlatformApi;

#include "renderer.h"
#include "ui.h"

// #include "text_buffer.h"

typedef struct _editor_context {
    renderer *Renderer;
    rv2 mPos;
    c8  LastChar;
    //todo: add buffer stuff, etc
} editor_context;

#define CMD_PROC(Name) void cmd_proc_##Name(editor_context *c)
typedef CMD_PROC(callback);

CMD_PROC(DoNothing) {

}

CMD_PROC(InsertChar) {
    DrawRect(c->Renderer, rect_(10, 20, 10, 10), (colorb){0xFFFFFFFF});
}

CMD_PROC(DeleteCharFoward) {

}

CMD_PROC(DeleteCharBackward) {

}

CMD_PROC(Indent) {

}

CMD_PROC(MoveCarretLeft) {

}

CMD_PROC(MoveCarretRight) {

}

CMD_PROC(MoreCarretUp) {

}

CMD_PROC(MoveCarretDown) {

}

CMD_PROC(MoveCarretToLineStart) {

}

CMD_PROC(MoveCarretToLineEnd) {

}

CMD_PROC(InsertNewLine) {

}

CMD_PROC(SaveFile) {

}

CMD_PROC(OpenFile) {
    DrawRect(c->Renderer, rect_(10, 10, 10, 10), (colorb){0xFFFFFFFF});
}

typedef struct _command {
    cmd_proc_callback *Proc;
    c8                *Desc;
} command;
#define command_(Proc, Desc) (command){(Proc), (Desc)}

typedef enum _key {
    KEY_NONE = 0, //note:
    KEY_CHAR = 1, //note:
    KEY_DEL,
    KEY_BACK,
    KEY_TAB,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_PG_UP,
    KEY_PG_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_RETURN,
} key;


#define KeyComb(BaseKey, Ctrl, Alt, Shift) (u16)(BaseKey) | ((u16)(Ctrl) << 8) | ((u16)(Alt)  << 9) | ((u16)(Shift) << 10)
#define Ctrl(Key)  KeyComb(Key, 1, 0, 0)
#define Alt(Key)   KeyComb(Key, 0, 1, 0)
#define Shift(Key) KeyComb(Key, 0, 0, 1)

internal u16 GetPlatformKeyboardKeyComb(platform *p, editor_context *c) {
    u16 Key = KEY_NONE;
    if (p->kDelete)
        Key = KeyComb(KEY_DEL, p->kCtrl, p->kAlt, p->kShift);
    if (p->kBack)
        Key = KeyComb(KEY_BACK, p->kCtrl, p->kAlt, p->kShift);
    if (p->kTab)
        Key = KeyComb(KEY_TAB, p->kCtrl, p->kAlt, p->kShift);
    if (p->kLeft)
        Key = KeyComb(KEY_LEFT, p->kCtrl, p->kAlt, p->kShift);
    if (p->kRight)
        Key = KeyComb(KEY_RIGHT, p->kCtrl, p->kAlt, p->kShift);
    if (p->kUp)
        Key = KeyComb(KEY_UP, p->kCtrl, p->kAlt, p->kShift);
    if (p->kDown)
        Key = KeyComb(KEY_DOWN, p->kCtrl, p->kAlt, p->kShift);
    if (p->kHome)
        Key = KeyComb(KEY_HOME, p->kCtrl, p->kAlt, p->kShift);
    if (p->kPgUp)
        Key = KeyComb(KEY_PG_UP, p->kCtrl, p->kAlt, p->kShift);
    if (p->kPgDown)
        Key = KeyComb(KEY_PG_DOWN, p->kCtrl, p->kAlt, p->kShift);
    if (p->kEnd)
        Key = KeyComb(KEY_END, p->kCtrl, p->kAlt, p->kShift);
    if (p->kReturn)
        Key = KeyComb(KEY_RETURN, p->kCtrl, p->kAlt, p->kShift);
    if (p->kChar) 
        Key = KEY_CHAR;
    c->LastChar = p->Char;
    return Key;
}

global command Keymap[1024];

///////////////////////////////////////////////////////////

void DrawUi(editor_context *c) {
    DrawRect(c->Renderer, rect_(GetVecComps(c->mPos), 100, 100), (colorb){0xFFFFFFFF});
}

typedef struct _app_state {
    renderer *Renderer;

    ui_style UiStyle;
    ui_ctx   UiContext;

    command *Keymap;

    i16 dLastMouseWheel;
} app_state;

external APP_INIT(Init) {
    Assert(sizeof(app_state) <= p->Memory.Size);
    app_state *State = (app_state *)p->Memory.Contents;

    platform_api PlatformApi;

    PlatformApi.AllocateMemory         = p->AllocateMemoryCallback;
    PlatformApi.FreeMemory             = p->FreeMemoryCallback;
    PlatformApi.LoadFile               = p->LoadFileCallback;
    PlatformApi.FreeFile               = p->FreeFileCallback;
    PlatformApi.LoadFileToArena        = p->LoadFileToArenaCallback;
    PlatformApi.FreeFileFromArena      = p->FreeFileFromArenaCallback;
    PlatformApi.WriteFile              = p->WriteFileCallback;
    PlatformApi.ReportError            = p->ReportErrorCallback;
    PlatformApi.ReportErrorAndDie      = p->ReportErrorAndDieCallback;

    GlobalPlatformApi = PlatformApi;

    State->Renderer = PlatformApi.AllocateMemory(sizeof(renderer));
    
    State->UiContext.Hot     = -1;
    State->UiContext.Clicked = -1;
    State->UiContext.Last    = -1;
    State->UiContext.Current = -1;

    State->UiStyle.Font    = LoadFont(State->Renderer, &PlatformApi, "roboto_mono.ttf", 400, 24);
    State->UiStyle.Padding = rv2_(20, 20);
    State->UiStyle.DefaultTextColor   = (colorb){0xFAFAFAFF};
    State->UiStyle.HotButtonColor     = (colorb){0x606060FF};
    State->UiStyle.ClickedButtonColor = (colorb){0x808080FF};
    State->UiStyle.DefaultButtonColor = (colorb){0x404040FF};
    State->UiStyle.CharSpacing = 0;
    State->UiStyle.LineSpacing = 0;
    State->UiStyle.SliderHandleHeight = 20;
    State->UiStyle.SliderHandleWidth  = 10;

    Keymap[KEY_NONE] = command_(cmd_proc_DoNothing, "DoNothing");
    Keymap[KEY_CHAR] = command_(cmd_proc_InsertChar, "InsertChar");
    Keymap[KEY_DEL]  = command_(cmd_proc_DeleteCharFoward, "DeleteCharFoward");
    Keymap[KEY_BACK] = command_(cmd_proc_DeleteCharBackward, "DeleteCharBackward");
    Keymap[KEY_TAB]  = command_(cmd_proc_Indent, "Indent");
    Keymap[KEY_LEFT]  = command_(cmd_proc_MoveCarretLeft, "MoveCarretLeft");
    Keymap[KEY_RIGHT] = command_(cmd_proc_MoveCarretRight, "MoveCarretRight");
    Keymap[KEY_UP]    = command_(cmd_proc_MoreCarretUp, "MoreCarretUp");
    Keymap[KEY_DOWN]  = command_(cmd_proc_MoveCarretDown, "MoveCarretDown");
    Keymap[KEY_HOME]  = command_(cmd_proc_MoveCarretToLineStart, "MoveCarretToLineStart");
    Keymap[KEY_END]   = command_(cmd_proc_MoveCarretToLineEnd, "MoveCarretToLineEnd");
    Keymap[KEY_RETURN] = command_(cmd_proc_InsertNewLine, "InsertNewLine");
    // Keymap[Ctrl('S')] = command_(cmd_proc_SaveFile, "SaveFile");
    // Keymap[Ctrl('O')] = command_(cmd_proc_OpenFile, "OpenFile");    

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    i16 dMouseWheel        = (State->dLastMouseWheel != p->dmWheel)? p->dmWheel : 0;
    State->dLastMouseWheel = p->dmWheel;

    State->UiContext.dmWheel           = dMouseWheel;
    State->UiContext.mPos              = p->mPos;
    State->UiContext.mLeftButtonIsDown = p->mLeft;

    editor_context Context;
    Context.Renderer = State->Renderer;
    Context.mPos     = p->mPos;

    if (Keymap[GetPlatformKeyboardKeyComb(p, &Context)].Proc)
        Keymap[GetPlatformKeyboardKeyComb(p, &Context)].Proc(&Context);

    DrawUi(&Context);
    //architecture:
    //  the ui's input events (button presses, etc) are written into
    //  the 'Context', for the next frame to resolve it's shit

    Render(State->Renderer, p->WindowDim, (colorb){0x202020FF});
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;

    GlobalPlatformApi.AllocateMemory    = p->AllocateMemoryCallback;
    GlobalPlatformApi.FreeMemory        = p->FreeMemoryCallback;
    GlobalPlatformApi.LoadFile          = p->LoadFileCallback;
    GlobalPlatformApi.FreeFile          = p->FreeFileCallback;
    GlobalPlatformApi.WriteFile         = p->WriteFileCallback;
    GlobalPlatformApi.ReportError       = p->ReportErrorCallback;
    GlobalPlatformApi.ReportErrorAndDie = p->ReportErrorAndDieCallback;
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}

#if 0
#endif
