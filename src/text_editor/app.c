#include "lingo.h"
#include "platform.h"
#include "renderer.h"
#include "colors.h"
#include "gbuff.h"
#include "ui.h"

typedef struct _keybind_command_context {
    gbuff *CurrentBuff;
    platform_api *Api;
    u32 Col;
    u32 Line;
    c8 Char;
} keybind_command_context;

#define KEYBIND_COMMAND(Name) void keybind_command_##Name(keybind_command_context *Ctx)
typedef KEYBIND_COMMAND(callback);

typedef struct _keybind_command {
    keybind_command_callback *Proc;
} keybind_command;
#define keybind_command_(Proc) (keybind_command){(keybind_command_##Proc)}

typedef enum _keybind_keys {
    keybind_KEY_NONE = 0, //note:
    keybind_KEY_CHAR = 1, //note:
    keybind_KEY_DEL,
    keybind_KEY_BACK,
    keybind_KEY_TAB,
    keybind_KEY_LEFT,
    keybind_KEY_RIGHT,
    keybind_KEY_UP,
    keybind_KEY_DOWN,
    keybind_KEY_PG_UP,
    keybind_KEY_PG_DOWN,
    keybind_KEY_HOME,
    keybind_KEY_END,
    keybind_KEY_RETURN,
    keybind_KEY_CTRL  = 1 << 8,
    keybind_KEY_ALT   = 1 << 9,
    keybind_KEY_SHIFT = 1 << 10
} keybind_keys;

#define KeyComb(Base, Ctrl, Alt, Shift) \
    (u16)(Base)        |                \
   ((u16)(Ctrl)  << 8) |                \
   ((u16)(Alt)   << 9) |                \
   ((u16)(Shift) << 10)

typedef struct _app_state {
    memory_arena Arena;
    renderer   Renderer;
    ui_context ui_Context;
    gbuff Buff;
    rect  Caret;
    u32 Col;
    u32 Line;
    keybind_command Keymap[1024];
} app_state;

KEYBIND_COMMAND(DoNothing) {
    return;
}

KEYBIND_COMMAND(BuffInsertChar) {
    if (Ctx->Char != '\b' && Ctx->Char != '\r')
        gbuff_InsertChar(Ctx->Api, Ctx->CurrentBuff, Ctx->CurrentBuff->Point, Ctx->Char);
}

KEYBIND_COMMAND(BuffMoveCursorLeft) {
    gbuff *Buff = Ctx->CurrentBuff;
    Buff->Point = gbuff_GetPrevCharCursor(Buff, Buff->Point);
    Ctx->Col = gbuff_GetColumn(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffMoveCursorRight) {
    gbuff *Buff = Ctx->CurrentBuff;
    Buff->Point = gbuff_GetNextCharCursor(Buff, Buff->Point);
    Ctx->Col = gbuff_GetColumn(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffMoveCursorUp) {
    gbuff *Buff = Ctx->CurrentBuff;
    u32 BeginningOfPrevLine = gbuff_GetBeginningOfPrevLineCursor(Buff, Buff->Point);
    u32 PrevLineLen         = gbuff_GetLineLen(Buff, BeginningOfPrevLine);
    Buff->Point = BeginningOfPrevLine + Min(PrevLineLen,  Ctx->Col);
    Ctx->Line =  Ctx->Line > 0?  Ctx->Line - 1 : 0;
}

KEYBIND_COMMAND(BuffMoveCursorDown) {
    gbuff *Buff = Ctx->CurrentBuff;
    u32 BeginningOfNextLine = gbuff_GetBeginningOfNextLineCursor(Buff, Buff->Point);
    u32 NextLineLen         = gbuff_GetLineLen(Buff, BeginningOfNextLine);
    Buff->Point = BeginningOfNextLine + Min(NextLineLen,  Ctx->Col);
    Ctx->Line++;
}

KEYBIND_COMMAND(BuffMoveCursorToLineStart) {
    gbuff *Buff = Ctx->CurrentBuff;
    Buff->Point = gbuff_GetBeginningOfLineCursor(Buff, Buff->Point);
    Ctx->Col = gbuff_GetColumn(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffMoveCursorToLineEnd) {
    gbuff *Buff = Ctx->CurrentBuff;
    Buff->Point = gbuff_GetEndOfLineCursor(Buff, Buff->Point);
    Ctx->Col = gbuff_GetColumn(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffDeleteFowardChar) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_DeleteBackwardChar(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffDeleteBackwardChar) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_DeleteFowardChar(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffNewline) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_InsertChar(Ctx->Api, Buff, Buff->Point, '\n');
    Ctx->Line++;
}

KEYBIND_COMMAND(BuffOpen) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_Load(Ctx->Api, Buff, "a.c");
}

KEYBIND_COMMAND(BuffSave) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_Save(Ctx->Api, Buff, "a.c");
}


external APP_INIT(Init) {
    app_state *s = (app_state *)p->Memory.Contents;

    s->Arena = InitializeArena(p->Memory.Size - sizeof(app_state), (u8 *)p->Memory.Contents + sizeof(app_state));

    platform_api           *Api = &p->Api;
    memory_arena *Arena    = &s->Arena;
    renderer     *Renderer = &s->Renderer;

    s->ui_Context.Hover   =  0;
    s->ui_Context.Focus   =  0;
    s->ui_Context.LastId  =  0;
    s->ui_Context.Current = -1;

    Renderer->Fonts[0] = LoadFont(Api, Arena, "roboto.ttf", 16);
    s->Buff  = gbuff_Create(Api, 2);
    s->Caret = rect_(-100000, -100000, -100000, -100000);
    s->Col   = 0;
    s->Line  = 0;

    s->Keymap[keybind_KEY_NONE] = keybind_command_(DoNothing);
    s->Keymap[keybind_KEY_CHAR] = keybind_command_(BuffInsertChar);
    s->Keymap[keybind_KEY_LEFT]  = keybind_command_(BuffMoveCursorLeft);
    s->Keymap[keybind_KEY_RIGHT] = keybind_command_(BuffMoveCursorRight);
    s->Keymap[keybind_KEY_UP]   = keybind_command_(BuffMoveCursorUp);
    s->Keymap[keybind_KEY_DOWN] = keybind_command_(BuffMoveCursorDown);
    s->Keymap[keybind_KEY_HOME] = keybind_command_(BuffMoveCursorToLineStart);
    s->Keymap[keybind_KEY_END]  = keybind_command_(BuffMoveCursorToLineEnd);
    s->Keymap[keybind_KEY_BACK] = keybind_command_(BuffDeleteFowardChar);
    s->Keymap[keybind_KEY_DEL]  = keybind_command_(BuffDeleteBackwardChar);
    s->Keymap[keybind_KEY_RETURN] = keybind_command_(BuffNewline);
    s->Keymap[keybind_KEY_CTRL | 'S'] = keybind_command_(BuffOpen);
    s->Keymap[keybind_KEY_CTRL | 'O'] = keybind_command_(BuffSave);

    Api->Enable(GL_BLEND);
    Api->BlendFunc();
}

external APP_UPDATE(Update) {
    app_state *s = (app_state *)p->Memory.Contents;

    platform_api *Api      = &p->Api;
    memory_arena *Arena    = &s->Arena;
    renderer     *Renderer = &s->Renderer;

    s->ui_Context.MouseDown = p->Buttons[plat_KEYM_LEFT].EndedDown ||
                              p->Buttons[plat_KEYM_LEFT].EndedDown ||
                              p->Buttons[plat_KEYM_LEFT].EndedDown;
    s->ui_Context.MouseWich = p->Buttons[plat_KEYM_LEFT].EndedDown?  ui_MOUSE_BUTTON_LEFT  :
                              p->Buttons[plat_KEYM_RIGHT].EndedDown? ui_MOUSE_BUTTON_RIGHT : 0;
    s->ui_Context.MousePos = p->MousePos;

    s->ui_Context.Style.Padding = 16;
    s->ui_Context.Style.Font    = 0;
    s->ui_Context.Style.Colors[ui_COLOR_TEXT]         = GREY_100;
    s->ui_Context.Style.Colors[ui_COLOR_BACK]         = GREY_900;
    s->ui_Context.Style.Colors[ui_COLOR_BUTTON]       = INDIGO_500;
    s->ui_Context.Style.Colors[ui_COLOR_BUTTON_HOVER] = INDIGO_600;
    s->ui_Context.Style.Colors[ui_COLOR_BUTTON_FOCUS] = INDIGO_700;

    s->ui_Context.Layout.Body = rect_(100, 100, 200, 200);
    s->ui_Context.Layout.Max  = rv2_(-0x1000000, -0x1000000);
    s->ui_Context.Layout.Pos  = s->ui_Context.Layout.Body.Pos;
    s->ui_Context.Layout.Dim  = s->ui_Context.Layout.Body.Dim;
    s->ui_Context.Layout.ItemWidth  = 64;
    s->ui_Context.Layout.ItemHeight = 36;

    DrawRect(Renderer, rect_(16, s->Caret.y, 800, s->Caret.h), GREY_800);
    DrawRect(Renderer, s->Caret, ORANGE_700);
    gbuff_Render(Renderer, Api, &s->Buff, rv2_(16, p->WindowDim.y - 16), &s->Caret);

    ui_Label(Renderer, &s->ui_Context, "δένδρον", 0);
    if (ui_Button(Renderer, &s->ui_Context, "GREEK", 0));
    ui_NextRow(&s->ui_Context);

    ui_Label(Renderer, &s->ui_Context, "árvore", 0);
    if (ui_Button(Renderer, &s->ui_Context, "PORTUGUESE", 0));

    ui_NextRow(&s->ui_Context);
    ui_Label(Renderer, &s->ui_Context, "дерево", 0);
    if (ui_Button(Renderer, &s->ui_Context, "RUSSIAN", 0));
    ui_NextRow(&s->ui_Context);

    keybind_command_context CommandContext;
    CommandContext.CurrentBuff = &s->Buff;
    CommandContext.Api = Api;
    CommandContext.Col = 0;
    CommandContext.Line = 0;
    CommandContext.Char = p->Char;

    u16 Key = keybind_KEY_NONE;
    b32 Ctrl  = p->Buttons[plat_KEYB_CTRL].EndedDown;
    b32 Alt   = p->Buttons[plat_KEYB_ALT].EndedDown;
    b32 Shift = p->Buttons[plat_KEYB_SHIFT].EndedDown;

    if (p->Buttons[plat_KEYBEV_CHAR].EndedDown)
        Key = KeyComb(keybind_KEY_CHAR, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_LEFT].EndedDown)
        Key = KeyComb(keybind_KEY_LEFT, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_RIGHT].EndedDown)
        Key = KeyComb(keybind_KEY_RIGHT, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_UP].EndedDown)
        Key = KeyComb(keybind_KEY_UP, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_DOWN].EndedDown)
        Key = KeyComb(keybind_KEY_DOWN, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_HOME].EndedDown)
        Key = KeyComb(keybind_KEY_HOME, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_END].EndedDown)
        Key = KeyComb(keybind_KEY_END, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_BACK].EndedDown)
        Key = KeyComb(keybind_KEY_BACK, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_DELETE].EndedDown)
        Key = KeyComb(keybind_KEY_DEL, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_RETURN].EndedDown)
        Key = KeyComb(keybind_KEY_RETURN, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_CTRL].EndedDown && p->Char == 'O')
        Key = keybind_KEY_CTRL | 'S';
    else
    if (p->Buttons[plat_KEYB_CTRL].EndedDown && p->Char == 'S')
        Key = keybind_KEY_CTRL | 'O';

    if (s->Keymap[Key].Proc)
        s->Keymap[Key].Proc(&CommandContext);

    Render(Api, Renderer, p->WindowDim, s->ui_Context.Style.Colors[ui_COLOR_BACK]);
    DEBUG_DrawFontAtlas(s->Renderer.Fonts[0].Atlas);
}

external APP_RELOAD(Reload) {
    app_state *s = (app_state *)p->Memory.Contents;
}

external APP_DEINIT(Deinit) {
    app_state *s = (app_state *)p->Memory.Contents;
}

#if 0
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

#include "colors.h"
#include "app.h"

internal rv2 DrawToken(renderer *Renderer, token Token, buffer *Buffer, id Font, colorb Color, rv2 Pos, r32 InitialX) {
    font RenderFont = Renderer->Fonts[Font];

    if (Token.Error)
        Color = RED_900;

    for (cursor Cursor = Token.Start; Cursor < Token.End; Cursor++) {
        c8   Char      =  GetBufferChar(Buffer, Cursor);
        u32  Index     = (Char - 32 >= 0)? Char - 32 : '?' - 32;
        rv2  Offset    =  RenderFont.GlyphOffsets[Index];
        rect GlyphRect =  rect_(Pos.x + Offset.x, Pos.y - Offset.y, GetVecComps(RenderFont.GlyphRects[Index].Dim));

        if (Char == '\n') {
            Pos.y -= RenderFont.LineGap;
            Pos.x  = InitialX;
            continue;
        }
        else
        if (Char == '\r') {
            continue;
        }
        else
        if (Char == ' ') {
            Pos.x += RenderFont.GlyphAdvances[0];
            continue;
        }

        DrawGlyph(Renderer, Font, Index, GlyphRect.Pos, Color);

        Pos.x += RenderFont.GlyphAdvances[Index];
    }

    return Pos;
}

internal void DrawScannedBuffer(editor_context *c, buffer *Buffer) {
    // ui_style *Style    = c->uiStyle;
    // ui_input *Input    = c->uiInput;
    renderer *Renderer = c->Renderer;

    rv2   Pos   = rv2_(10, Renderer->TargetClipRect.h - (20 + Renderer->Fonts[0].Height));
    token Token = {0};
    
    colorb SyntaxColors[] = {
        [TOKEN_TYPE_OPERATOR]   = GREY_600,
        [TOKEN_TYPE_DELIMITER]  = GREY_600,
        [TOKEN_TYPE_KEYWORD]    = BLUE_600,
        [TOKEN_TYPE_STR]        = GREEN_600,
        [TOKEN_TYPE_INT]        = ORANGE_600,
        [TOKEN_TYPE_FLOAT]      = RED_500,
        [TOKEN_TYPE_NAME]       = GREY_50,
        [TOKEN_TYPE_COMMENT]    = GREEN_200
    };

    // r32  Ascender = c->Renderer->Fonts[/*c->uiStyle->MonoFont*/].Ascender;
    // rect Line  = rect_(/*c->uiStyle->Padding.x*/, c->CurrentCaretPos.y, c->Renderer->TargetClipRect.w/2 - /*c->uiStyle->Padding.x*/, Ascender);
    // rect Caret = rect_(c->CurrentCaretPos.x, c->CurrentCaretPos.y, 2, Ascender);
    // DrawRect(c->Renderer, Line,  GREY_800);
    // DrawRect(c->Renderer, Caret, GREY_50);

    cursor Cursor = NextToken(&GlobalPlatformApi, Buffer, 0, &Token);
    while (Token.Type != TOKEN_TYPE_EOF) {
        Pos    = DrawToken(Renderer, Token, Buffer, /*Style->MonoFont*/ 0, SyntaxColors[Token.Type], Pos, 10/*Style->Padding.x*/);
        Cursor = NextToken(&GlobalPlatformApi, Buffer, Cursor, &Token);
    }
}

/*internal void DrawBottomBar(editor_context *c) {
    buffer   *Buffer   = c->Buffers[c->CurrentBuffer];
    ui_ctx   *Ctx      = c->uiCtx;
    ui_style *Style    = c->uiStyle;
    ui_input *Input    = c->uiInput;
    renderer *Renderer = c->Renderer;

    rect   BarBackground = {
        .w   = Renderer->TargetClipRect.w,
        .h   = Style->Padding.y*2 + Renderer->Fonts[Style->MainFont].Height,
        .Pos = rv2_(0, 0)
    };

    DrawRect(Renderer, BarBackground, Style->DefaultColor);

    r32 x = 0;
    uiButton(Ctx, Style, Input, Renderer, rv2_(x, 0), c->Filename);
    x += MeasureText(Renderer, c->Filename, Style->MainFont, rv2_(0, 0)).w + Style->Padding.x*2;
    c8 TextBuffer[32];
    sprintf_s(TextBuffer, 32, "%u, %u, %u", c->CurrentBufferLine, c->CurrentBufferColumn, Buffer->Point);
    uiButton(Ctx, Style, Input, Renderer, rv2_(x, 0), TextBuffer);
    x = Renderer->TargetClipRect.w - MeasureText(Renderer, "00.000000", Style->MainFont, rv2_(0, 0)).w - Style->Padding.x*2;
    sprintf_s(TextBuffer, 32, "%f", 1.f/c->dtFrame);
    uiButton(Ctx, Style, Input, Renderer, rv2_(x, 0), TextBuffer);
}*/

global command Keymap[1024];

inline void SetKeymap() {
    Keymap[KEY_NONE] = command_(cmd_proc_DoNothing, "DoNothing");
    Keymap[KEY_CHAR] = command_(cmd_proc_InsertChar, "InsertChar");
    Keymap[KEY_DEL]  = command_(cmd_proc_DeleteCharFoward, "DeleteCharFoward");
    Keymap[KEY_BACK] = command_(cmd_proc_DeleteCharBackward, "DeleteCharBackward");
    Keymap[KEY_TAB]  = command_(cmd_proc_Indent, "Indent");
    Keymap[KEY_LEFT]  = command_(cmd_proc_MoveCarretLeft, "MoveCarretLeft");
    Keymap[KEY_RIGHT] = command_(cmd_proc_MoveCarretRight, "MoveCarretRight");
    Keymap[KEY_CTRL | KEY_LEFT]  = command_(cmd_proc_MoveCarretToPrevToken, "MoveCarretToPrevToken");
    Keymap[KEY_CTRL | KEY_RIGHT] = command_(cmd_proc_MoveCarretToNextToken, "MoveCarretToNextToken");
    Keymap[KEY_UP]   = command_(cmd_proc_MoveCarretUp, "MoreCarretUp");
    Keymap[KEY_DOWN] = command_(cmd_proc_MoveCarretDown, "MoveCarretDown");
    Keymap[KEY_HOME] = command_(cmd_proc_MoveCarretToLineStart, "MoveCarretToLineStart");
    Keymap[KEY_END]  = command_(cmd_proc_MoveCarretToLineEnd, "MoveCarretToLineEnd");
    Keymap[KEY_CTRL | KEY_HOME] = command_(cmd_proc_MoveCarretToBufferStart, "MoveCarretToLineStart");
    Keymap[KEY_CTRL | KEY_END]  = command_(cmd_proc_MoveCarretToBufferEnd,   "MoveCarretToLineEnd");
    Keymap[KEY_RETURN] = command_(cmd_proc_InsertNewLine, "InsertNewLine");
    Keymap[KEY_CTRL | 'S'] = command_(cmd_proc_SaveFile, "SaveFile");
    Keymap[KEY_CTRL | 'O'] = command_(cmd_proc_OpenFile, "OpenFile");
}

inline void SetApi(platform *p) {
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
    renderer       Renderer;
    editor_context Context;
    memory_arena   Arena;
} app_state;

external APP_INIT(Init) {
    Assert(sizeof(app_state) <= p->Memory.Size);
    app_state *State = (app_state *)p->Memory.Contents;

    SetApi(p);

    LoadFont(&State->Renderer, &GlobalPlatformApi, "roboto_mono.ttf", 400, 32);

    State->Arena = InitializeArena(p->Memory.Size     - sizeof(app_state),
                             (u8 *)p->Memory.Contents + sizeof(app_state));
    State->Context.Renderer = &State->Renderer;
    State->Context.Buffers = PushToArena(&State->Arena, sizeof(buffer *));
    State->Context.Buffers[0] = CreateBuffer(8);

    LoadBuffer(State->Context.Buffers[0], "a.c");

#if 0
    // State->Context.uiCtx   = PushToArena(&State->Arena, sizeof(ui_ctx));
    // State->Context.uiStyle = PushToArena(&State->Arena, sizeof(ui_style));
    // State->Context.uiInput = PushToArena(&State->Arena, sizeof(ui_input));

    // State->Context.uiCtx->Hot      = -1;
    // State->Context.uiCtx->Clicked  = -1;
    // State->Context.uiCtx->Last     = -1;
    // State->Context.uiCtx->Current  = -1;
    // State->Context.uiCtx->NoItems  = 256;

    // State->Context.uiStyle->MainFont = LoadFont(State->Context.Renderer, &GlobalPlatformApi, "roboto.ttf", 400, 24);
    // State->Context.uiStyle->MonoFont = LoadFont(State->Context.Renderer, &GlobalPlatformApi, "roboto_mono.ttf", 400, 24);
    // State->Context.uiStyle->IconFont = LoadFont(State->Context.Renderer, &GlobalPlatformApi, "icons.ttf", 400, 24);
    // State->Context.uiStyle->Padding  = rv2_(10, 10);
    // State->Context.uiStyle->ClickedColor = GREY_600;
    // State->Context.uiStyle->HotColor     = GREY_700;
    // State->Context.uiStyle->DefaultColor = GREY_800;
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    // UpdateEditorContextInput(&State->Context, p);
    DrawScannedBuffer(&State->Context, State->Context.Buffers[State->Context.CurrentBuffer]);
    // DrawBottomBar(&State->Context);
    // if (Keymap[State->Context.LastKeyComb].Proc)
    //     Keymap[State->Context.LastKeyComb].Proc(&State->Context);
    Render(&State->Renderer, p->WindowDim, GREY_900);
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;

    SetApi(p);
    SetKeymap();
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}
#endif