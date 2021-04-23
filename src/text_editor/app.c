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
    //todo: add buffer stuff, etc
} editor_context;

#define CMD_PROC(Name) void cmd_proc_##Name(editor_context *c)
typedef CMD_PROC(callback);

CMD_PROC(InsertChar) {

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

}

typedef struct _command {
    cmd_proc_callback *Proc;
    c8                *Desc;
} command;
#define command_(Proc, Desc) (command){(Proc), (Desc)}

typedef enum _key {
    KEY_NONE = 0,
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
    KEY_CHAR,
} key;

internal finginline u16 GetKeyComb(/*note: how should i do this?*/) {
    // return (u16)Key | ((u16)Ctrl << 8) | ((u16)Alt << 9) | ((u16)Shift << 10);
}

internal finginline u16 Ctrl(u8 Key) {
    // return GetKeyComb(1, 0, 0, Key);
}

internal finginline u16 Alt(u8 Key) {
    // return GetKeyComb(0, 1, 0, Key);
}

internal finginline u16 Sh1ft(u8 Key) {
    // return GetKeyComb(0, 0, 1, Key);
}

///////////////////////////////////////////////////////////

void DrawUi(editor_context *c) {
    DrawRect(c->Renderer, rect_(GetVecComps(c->mPos), 100, 100), (colorb){0xFFFFFFFF});
}

typedef struct _app_state {
    renderer *Renderer;

    ui_style UiStyle;
    ui_ctx   UiContext;

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

    // command Keymap[] = {
    //     [KEY_CHAR] = command_(cmd_proc_InsertChar,         "description"),
    //     [KEY_DEL]  = command_(cmd_proc_DeleteCharFoward,   "description"),
    //     [KEY_BACK] = command_(cmd_proc_DeleteCharBackward, "description"),
    //     [KEY_TAB]  = command_(cmd_proc_Indent,             "description"),
    //     [KEY_LEFT]  = command_(cmd_proc_MoveCarretLeft,        "description"),
    //     [KEY_RIGHT] = command_(cmd_proc_MoveCarretRight,       "description"),
    //     [KEY_UP]    = command_(cmd_proc_MoreCarretUp,          "description"),
    //     [KEY_DOWN]  = command_(cmd_proc_MoveCarretDown,        "description"),
    //     [KEY_HOME]  = command_(cmd_proc_MoveCarretToLineStart, "description"),
    //     [KEY_END]   = command_(cmd_proc_MoveCarretToLineEnd,   "description"),
    //     [KEY_RETURN] = command_(cmd_proc_InsertNewLine, "description"),
    //     // [Ctrl('S')] = command_(cmd_proc_SaveFile, "description"),
    //     // [Ctrl('O')] = command_(cmd_proc_OpenFile, "description")
    // };

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

    // Keymap[GetKeyComb(/*i don't know what i am doing*/)]->
    //     Command(&Context);

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
