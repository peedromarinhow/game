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

    renderer *Renderer;

    ui_style UiStyle;
    ui_ctx   UiContext;

    r32 Slider1;
    r32 Slider2;
    r32 Slider3;

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
    PlatformApi.GetAllFilenamesFromDir = p->GetAllFilenamesFromDir;
    PlatformApi.ReportError            = p->ReportErrorCallback;
    PlatformApi.ReportErrorAndDie      = p->ReportErrorAndDieCallback;

    GlobalPlatformApi = PlatformApi;

    State->Keymap = CreateMyKeymap();
    State->CommandContext.GoalColumn = -1;

    State->Renderer = PlatformApi.AllocateMemory(sizeof(renderer));

    State->RobotoMono = LoadFont(State->Renderer, &PlatformApi, "roboto_mono.ttf", 400, 24);
    State->Roboto     = LoadFont(State->Renderer, &PlatformApi, "roboto.ttf",      400, 32);

    State->UiContext.mPos = p->mPos;
    State->UiContext.mLeftButtonIsDown = p->mLeft;
    State->UiContext.Hot     = -1;
    State->UiContext.Clicked = -1;

    State->UiStyle.Font = State->RobotoMono;
    State->UiStyle.Padding = rv2_(20, 20);
 // State->UiStyle.HotTextColor     = (colorb){0xFF00AAFF};
 // State->UiStyle.ClickedTextColor = (colorb){0xFFAA00FF};
    State->UiStyle.DefaultTextColor = (colorb){0xFAFAFAFF};
    State->UiStyle.HotButtonColor     = (colorb){0x606060FF};
    State->UiStyle.ClickedButtonColor = (colorb){0x808080FF};
    State->UiStyle.DefaultButtonColor = (colorb){0x404040FF};
    State->UiStyle.CharSpacing = 0;
    State->UiStyle.LineSpacing = 0;
    State->UiStyle.SliderHandleHeight = 20;
    State->UiStyle.SliderHandleWidth  = 10;

    State->Slider1 = 1.f;
    State->Slider2 = 1.f;
    State->Slider3 = 1.f;

    file_group FileGroup;
    FileGroup.Filenames = GlobalPlatformApi.GetAllFileNamesFromDir("./data");
    FileGroup.NoFiles   = ArrayCount(FileGroup.Filenames);
    r32 x = 0;
    for (u32 FileIndex = 0; FileIndex < FileGroup.NoFiles; FileIndex++) {
        if (UiAddButton(State->Renderer, &State->UiContext, &State->UiStyle, rv2_(x, 0), FileGroup.Filenames[FileIndex])) {
            State->CommandContext.CurrentBuffer = FileIndex;
            State->CommandContext.Buffers[State->CommandContext.CurrentBuffer]=
                CreateBuffer(8, FileGroup.Filenames[FileIndex]));
        }
        x += 150;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    i16 dMouseWheel = (State->dLastMouseWheel != p->dmWheel)? p->dmWheel : 0;
    State->dLastMouseWheel = p->dmWheel;

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

    State->UiContext.mPos              = p->mPos;
    State->UiContext.mLeftButtonIsDown = p->mLeft;
    State->UiContext.dmWheel           = dMouseWheel;
    State->UiContext.Current           = -1;

    if (UiAddButton(State->Renderer, &State->UiContext, &State->UiStyle, rv2_(0, 0), "buffer 1")) {
        LoadBuffer(State->CommandContext.Buffers[0]);
        State->CommandContext.CurrentBuffer = 0;
    }

    if (UiAddButton(State->Renderer, &State->UiContext, &State->UiStyle, rv2_(300, 0), "buffer 2")) {
        LoadBuffer(State->CommandContext.Buffers[1]);
        State->CommandContext.CurrentBuffer = 1;
    }

    DrawBuffer(State->Renderer,
               State->CommandContext.Buffers[State->CommandContext.CurrentBuffer],
              &State->UiContext,
              &State->UiStyle,
              rv2_(State->UiStyle.Padding.x,
                   p->WindowDim.y - State->UiStyle.Padding.y - State->Renderer->Fonts[State->UiStyle.Font].Ascender));
    
    State->Keymap->Commands[Key].Func(&State->CommandContext);
    
    State->Slider1 = UiAddSlider(State->Renderer, &State->UiContext, &State->UiStyle, State->Slider1, rv2_(100, 100), 100);
    State->Slider2 = UiAddSlider(State->Renderer, &State->UiContext, &State->UiStyle, State->Slider2, rv2_(100, 140), 100);
    State->Slider3 = UiAddSlider(State->Renderer, &State->UiContext, &State->UiStyle, State->Slider3, rv2_(100, 180), 100);

    State->UiStyle.DefaultTextColor.r = State->Slider1*255;
    State->UiStyle.DefaultTextColor.g = State->Slider2*255;
    State->UiStyle.DefaultTextColor.b = State->Slider3*255;

    Render(State->Renderer, p->WindowDim, (colorb){0x202020FF});
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

    State->CommandContext.Buffers[0] = CreateBuffer(8, "a.c");
    State->CommandContext.Buffers[1] = CreateBuffer(8, "b.c");

    State->Keymap = CreateMyKeymap();
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}

#if 0
#endif
