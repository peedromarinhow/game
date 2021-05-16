#include "lingo.h"
#include "platform.h"
#include "renderer.h"
#include "colors.h"
#include "gbuff.h"
#include "ui.h"
#include "keybinding.h"

internal void gbuff_Render(renderer *Renderer, platform_api *Api, ui_context *ui_Ctx,
                           gbuff *Buff, rv2 Pos, rv2 *Caret) {
    ui_style *Style = &ui_Ctx->Style;
    font *Font = &Renderer->Fonts[Style->MonoFont];

    DrawRect(Renderer, rect_(Style->Padding, Caret->y, 800, Font->Height), GREY_800);
    DrawRect(Renderer, rect_(Caret->x, Caret->y, 2, Font->Height), ORANGE_700);

    for (u32 Cursor = 0; Cursor <= gbuff_GetLen(Buff); Cursor++) {
        utf8_quartet Utf8Quartet = gbuff_GetUtf8Quartet(Buff, Cursor);
        u32 NoCodepointBytes = 0;
        u32 Codepoint = GetNextCodepoint(&Utf8Quartet.Utf8_0, &NoCodepointBytes);
        u32 Index = GetGlyphIndex(Font, Codepoint);
        if (Codepoint == 0x3f) NoCodepointBytes = 1;
    
        r32 Advance = Font->Advances[Index];
        rv2 Bearing = rv2_(Font->Bearings[Index].x,
                           Font->Bearings[Index].y);
        rect Rect = rect_(Font->Rects[Index].x, Font->Rects[Index].y,
                          Font->Rects[Index].w, Font->Rects[Index].h);
        rv2 GlyphPos = rv2_(Pos.x + Bearing.x, Pos.y - (Rect.h - Bearing.y) - ((Font->Ascender) + (Font->Descender))/4);
        
        if (Cursor == Buff->Point)
           *Caret = rv2_(Pos.x, Pos.y - Font->Height/2);

        if (Utf8Quartet.Utf8_0 == '\n') {
            Pos = rv2_(Style->Padding, Pos.y - Font->Height);
            continue;
        }
        if (Utf8Quartet.Utf8_0 == '\r')
            continue;
    
        if (Cursor == gbuff_GetLen(Buff))
            break;  //note: dumb

        DrawGlyph(Renderer, Style->MonoFont, Index, GlyphPos, Style->Colors[ui_COLOR_TEXT]/*, ScaleFactor*/);

        Pos.x  += (i32)Advance;
        Cursor += (NoCodepointBytes - 1);
    }
}

typedef struct _app_state {
    memory_arena Arena;
    renderer   Renderer;
    ui_context ui_Context;
    gbuff Buff;
    rv2 CaretPos;
    keybind_command Keymap[1024];
    keybind_command_context CommandContext;
} app_state;

external APP_INIT(Init) {
    app_state *s = (app_state *)p->Memory.Contents;

    s->Arena = InitializeArena(p->Memory.Size - sizeof(app_state), (u8 *)p->Memory.Contents + sizeof(app_state));

    platform_api *Api      = &p->Api;
    memory_arena *Arena    = &s->Arena;
    renderer     *Renderer = &s->Renderer;

    s->ui_Context.Hover   =  0;
    s->ui_Context.Focus   =  0;
    s->ui_Context.LastId  =  0;
    s->ui_Context.Current = -1;

    Renderer->Fonts[0] = LoadFont(Api, Arena, "roboto.ttf", 16);
    Renderer->Fonts[1] = LoadFont(Api, Arena, "roboto_mono.ttf", 16);
    s->Buff     = gbuff_Create(Api, 2);
    s->CaretPos = rv2_(-100000, -100000);

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

    s->CommandContext.CurrentBuff = &s->Buff;
    s->CommandContext.Api = Api;
    s->CommandContext.Col = 0;
    s->CommandContext.Line = 0;
    s->CommandContext.Char = p->Char;

    Api->Enable(GL_BLEND);
    Api->BlendFunc();
}

external APP_UPDATE(Update) {
    app_state *s = (app_state *)p->Memory.Contents;

    platform_api *Api      = &p->Api;
    memory_arena *Arena    = &s->Arena;
    renderer     *Renderer = &s->Renderer;

    s->ui_Context.MouseDown = p->Buttons[plat_KEYM_LEFT] ||
                              p->Buttons[plat_KEYM_LEFT] ||
                              p->Buttons[plat_KEYM_LEFT];
    s->ui_Context.MouseWich = p->Buttons[plat_KEYM_LEFT]?  ui_MOUSE_BUTTON_LEFT  :
                              p->Buttons[plat_KEYM_RIGHT]? ui_MOUSE_BUTTON_RIGHT : 0;
    s->ui_Context.MousePos = p->MousePos;

    s->ui_Context.Style.Padding = 16;
    s->ui_Context.Style.Font     = 0;
    s->ui_Context.Style.MonoFont = 1;
    s->ui_Context.Style.Colors[ui_COLOR_TEXT]         = GREY_50;
    s->ui_Context.Style.Colors[ui_COLOR_BACK]         = GREY_900;
    s->ui_Context.Style.Colors[ui_COLOR_BUTTON]       = INDIGO_500;
    s->ui_Context.Style.Colors[ui_COLOR_BUTTON_HOVER] = INDIGO_600;
    s->ui_Context.Style.Colors[ui_COLOR_BUTTON_FOCUS] = INDIGO_700;

    s->ui_Context.Layout.Body = rect_(0, 0, 200, 200);
    s->ui_Context.Layout.Max  = rv2_(-0x1000000, -0x1000000);
    s->ui_Context.Layout.Pos  = s->ui_Context.Layout.Body.Pos;
    s->ui_Context.Layout.Dim  = s->ui_Context.Layout.Body.Dim;
    s->ui_Context.Layout.ItemWidth  = 64;
    s->ui_Context.Layout.ItemHeight = 36;

    gbuff_Render(Renderer, Api, &s->ui_Context, &s->Buff, rv2_(16, p->WindowDim.y - 16), &s->CaretPos);

    ui_Label(Renderer, &s->ui_Context, "δένδρον", 0);
    if (ui_Button(Renderer, &s->ui_Context, "GREEK", 0));
    ui_NextRow(&s->ui_Context);

    ui_Label(Renderer, &s->ui_Context, "árvore", 0);
    if (ui_Button(Renderer, &s->ui_Context, "PORTUGUESE", 0));

    ui_NextRow(&s->ui_Context);
    ui_Label(Renderer, &s->ui_Context, "дерево", 0);
    if (ui_Button(Renderer, &s->ui_Context, "RUSSIAN", 0));
    ui_NextRow(&s->ui_Context);

    u16 Key = keybind_KEY_NONE;
    b32 Ctrl  = p->Buttons[plat_KEYB_CTRL];
    b32 Alt   = p->Buttons[plat_KEYB_ALT];
    b32 Shift = p->Buttons[plat_KEYB_SHIFT];

    if (p->Buttons[plat_KEYBEV_CHAR])
        Key = KeyComb(keybind_KEY_CHAR, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_LEFT])
        Key = KeyComb(keybind_KEY_LEFT, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_RIGHT])
        Key = KeyComb(keybind_KEY_RIGHT, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_UP])
        Key = KeyComb(keybind_KEY_UP, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_DOWN])
        Key = KeyComb(keybind_KEY_DOWN, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_HOME])
        Key = KeyComb(keybind_KEY_HOME, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_END])
        Key = KeyComb(keybind_KEY_END, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_BACK])
        Key = KeyComb(keybind_KEY_BACK, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_DELETE])
        Key = KeyComb(keybind_KEY_DEL, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_RETURN])
        Key = KeyComb(keybind_KEY_RETURN, Ctrl, Alt, Shift);
    else
    if (p->Buttons[plat_KEYB_CTRL] && p->Char == 'O')
        Key = keybind_KEY_CTRL | 'S';
    else
    if (p->Buttons[plat_KEYB_CTRL] && p->Char == 'S')
        Key = keybind_KEY_CTRL | 'O';

    s->CommandContext.Char = p->Char;

    if (s->Keymap[Key].Proc)
        s->Keymap[Key].Proc(&s->CommandContext);

    Render(Api, Renderer, p->WindowDim, s->ui_Context.Style.Colors[ui_COLOR_BACK]);
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