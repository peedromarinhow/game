#include "lingo.h"
#include "platform.h"
#include "maths.h"

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
