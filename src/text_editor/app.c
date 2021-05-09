#include "lingo.h"
#include "platform.h"
#include "renderer.h"

#include "colors.h"

//note: inspired by github.com/rxi/microui

//todo:
//  id hashing
//  general input (including keyboard)

enum layout_type {
    ui_LAYOUT_ABSOLUTE,
    ui_LAYOUT_RELATIVE
};

typedef struct _ui_layout {
    rect Body;
    i32 ItemWidth;
    i32 ItemHeight;
    rv2 Pos;
    rv2 Dim;
    rv2 Max;
} ui_layout;

typedef struct _ui_style {
    id  Font;
    i32 Padding;
    colorb Colors[16];
} ui_style;

typedef struct _ui_context {
    ui_style Style;
    id Hover;
    id Focus;
    id LastId;
    id Current;
    i16 NoItems;
    b32 MouseDown;
    u32 MouseWich;
    b32 KeyDown;
    u32 KeyWich;
    rv2 MousePos;
    ui_layout Layout;
    c8 TextInput[32];
} ui_context;

enum ui_opt {
    ui_OPT_TEXT_ALIGN_CENTER = 0b1,
    ui_OPT_TEXT_ALIGN_RIGHT  = 0b10,
    ui_OPT_NO_INTERACTION    = 0b100,
    ui_OPT_HOLDFOCUS         = 0b1000
};

enum ui_result {
    ui_RESULT_CHANGE = 0b1,
    ui_RESULT_SUBMIT = 0b10
};

enum ui_color {
    ui_COLOR_BACK,
    ui_COLOR_BASE,
    ui_COLOR_TEXT,
    ui_COLOR_TEXT_INCATIVE,
    ui_COLOR_TEXT_FOCUS,
    ui_COLOR_BUTTON,
    ui_COLOR_BUTTON_HOVER, 
    ui_COLOR_BUTTON_FOCUS
};

enum ui_min_dims {
    ui_MIN_BUTTON_WIDTH  = 64,
    ui_MIN_BUTTON_HEIGHT = 36,
    ui_MIN_TEXTBOX_WIDTH  = 280,
    ui_MIN_TEXTBOX_HEIGHT = 48,
    ui_MIN_SNACKBAR_WIDTH  = 344,
    ui_MIN_SNACKBAR_HEIGHT = 48
};

enum ui_mouse_buttons {
    ui_MOUSE_BUTTON_LEFT  = 0b1,
    ui_MOUSE_BUTTON_RIGHT = 0b10
};

enum ui_key {
    ui_KEY_BACKSPACE = 0b1,
    ui_KEY_RETURN    = 0b10
};

internal id ui_GetId(ui_context *Ctx) {
    id Result = Ctx->Current;
    Ctx->LastId = Result;
    if (Ctx->Current < Ctx->NoItems)
        Ctx->Current++;
    return Result;
}

internal colorb ui_GetColor(ui_context *Ctx, id ColorId) {
    return Ctx->Style.Colors[ColorId];
}

internal void ui_NextRow(ui_context *Ctx) {
    ui_layout *Layout = &Ctx->Layout;
    Layout->Pos.y -= Layout->ItemHeight + Ctx->Style.Padding;
    Layout->Pos.x  = Layout->Body.x;
}

internal rect ui_NextRect(ui_context *Ctx, i32 Width, i32 Height) {
    ui_layout *Layout = &Ctx->Layout;
    ui_style  *Style  = &Ctx->Style;
    rect Result;

    Result.w = Layout->ItemWidth  = Width;
    Result.h = Layout->ItemHeight = Height;

    Result.x = Layout->Pos.x;
    Result.y = Layout->Pos.y + Layout->Dim.h - Layout->ItemHeight;

    Layout->Pos.x += Result.w + Style->Padding;

    Layout->Max.x = Max(Layout->Max.x, Result.x + Result.w);
    Layout->Max.y = Max(Layout->Max.y, Result.y + Result.h);

    return Result;
}

internal void ui_UpdateControls(ui_context *Ctx, id Id, rect Rect, u32 Opts) {
    //todo: figure out how to snap focus and how to hold focus
    if (Opts & ui_OPT_NO_INTERACTION)
        return;

    b32 MouseIsOver = IsInsideRect(Ctx->MousePos, Rect);
    if (MouseIsOver)
        Ctx->Hover = Id;
    else
        Ctx->Hover = 0;

    if (Ctx->Focus == Id) {
        if (!MouseIsOver)
            Ctx->Focus = 0;
    }

    if (Ctx->Hover == Id) {
        if (Ctx->MouseDown)
            Ctx->Focus = Id;
        else
            Ctx->Focus = 0;
    }
}

internal void ui_DrawRect(renderer *Renderer, ui_context *Ctx, id Id, rect Rect, id ColorId, u32 Opts) {
    if (Id == Ctx->Focus)
        ColorId = ui_COLOR_BUTTON_FOCUS;
    else
    if (Id == Ctx->Hover)
        ColorId = ui_COLOR_BUTTON_HOVER;
    DrawRect(Renderer, Rect, ui_GetColor(Ctx, ColorId));
}

internal void ui_DrawText(renderer *Renderer, ui_context *Ctx, c8 *Text, rect Rect, id ColorId, u32 Opts) {
    rv2 Dim = MeasureText(Renderer, Text, Ctx->Style.Font);
    rv2 Pos;
    Pos.y = Rect.y + (Rect.h - Dim.h) / 2;
    if (Opts & ui_OPT_TEXT_ALIGN_CENTER)
        Pos.x = Rect.x + (Rect.w - Dim.w) / 2;
    else
    if (Opts & ui_OPT_TEXT_ALIGN_RIGHT)
        Pos.x = (Rect.x + Rect.w) - Dim.w - Ctx->Style.Padding;
    else
        Pos.x = Rect.x + Ctx->Style.Padding;

    // if (Dim.w + Ctx->Style.Padding * 2 < Rect.w)
    //todo: handle text overflow i.e: long_text -> long_te..
    DrawText(Renderer, Text, Ctx->Style.Font, Pos, ui_GetColor(Ctx, ColorId));
}

internal void ui_Label(renderer* Renderer, ui_context *Ctx, c8 *Text, u32 Opts) {
    rect Rect = ui_NextRect(Ctx, ui_MIN_BUTTON_WIDTH, ui_MIN_BUTTON_HEIGHT);
    ui_DrawText(Renderer, Ctx, Text, Rect, ui_COLOR_TEXT, Opts);
}

internal b32 ui_Button(renderer* Renderer, ui_context *Ctx, c8 *Text, u32 Opts) {
    b32 Result = 0;
    
    id Id = ui_GetId(Ctx);
    rect Rect = ui_NextRect(Ctx, ui_MIN_BUTTON_WIDTH, ui_MIN_BUTTON_HEIGHT);

    ui_UpdateControls(Ctx, Id, Rect, 0);

    if (Ctx->MouseDown && Ctx->Focus == Id)
        Result = 1;

    ui_DrawRect(Renderer, Ctx, Id, Rect, ui_COLOR_BUTTON, Opts);
    if (Text)
        ui_DrawText(Renderer, Ctx, Text, Rect, ui_COLOR_TEXT, Opts | ui_OPT_TEXT_ALIGN_CENTER);
    return Result;
}

internal void ui_TextInput(ui_context *Ctx, c8 *Text) {
    u32 Len  = ArrayCount(Ctx->TextInput);
    u32 Size = ArrayCount(Text) + 1;
    Assert(Len + Size <= sizeof(Ctx->TextInput));
    CopyMemory(Ctx->TextInput + Len, Text, Size);
}

internal b32 ui_TextBox(renderer *Renderer, ui_context *Ctx, c8 *Buff, u32 BuffLen, u32 Opts) {
    b32 Result = 0;

    id Id = ui_GetId(Ctx);
    rect Rect = ui_NextRect(Ctx, ui_MIN_TEXTBOX_WIDTH, ui_MIN_TEXTBOX_HEIGHT);

    ui_UpdateControls(Ctx, Id, Rect, Opts | ui_OPT_HOLDFOCUS);

    if (Ctx->Focus == Id) {
        u32 Len = ArrayCount(Buff);
        u32 n = Min(BuffLen - Len - 1, ArrayCount(Ctx->TextInput));
        if (n > 0) {
            CopyMemory(Buff + Len, Ctx->TextInput, n);
            Len += n;
            Buff[Len] = '\0';
            Result |= ui_RESULT_CHANGE;
        }
        if (Ctx->KeyWich & ui_KEY_BACKSPACE && Len > 0) {
            while ((Buff[--Len] & 0xC0) == 0x80 && Len > 0);
            Buff[Len] = '\0';
            Result |= ui_RESULT_CHANGE;
        }
        if (Ctx->KeyWich & ui_KEY_RETURN) {
            // Ctx->Focus = 0;
            Result |= ui_RESULT_SUBMIT;
        }
    }

    ui_DrawRect(Renderer, Ctx, Id, rect_(Rect.x, Rect.y, Rect.w, 2), ui_COLOR_BUTTON, Opts);
    if (Ctx->Focus == Id) {
        ui_style *Style = &Ctx->Style;
        colorb Color = Style->Colors[ui_COLOR_TEXT];
        id  Font = Style->Font;
        rv2 Dim = MeasureText(Renderer, Buff, Font);
        rv2 Pos = rv2_(Rect.x + Min(Rect.w - Style->Padding - Dim.w - 1, Style->Padding),
                       Rect.y + (Rect.h - Dim.h) / 2);
        DrawText(Renderer, Buff, Font, Pos, Color);
        DrawRect(Renderer, rect_(Pos.x + Dim.w, Pos.y, 1, Dim.h), Color);
    }
    else {
        ui_DrawText(Renderer, Ctx, "Temp", Rect, ui_COLOR_TEXT, Opts);
    }

    return Result;
}

internal void ui_Snackbar(renderer* Renderer, ui_context *Ctx, c8 *Text, u32 Opts) {
    b32 Result = 0;

    rect Rect = rect_((Renderer->TargetClipRect.w - ui_MIN_SNACKBAR_WIDTH)/2, Ctx->Style.Padding, ui_MIN_SNACKBAR_WIDTH, ui_MIN_SNACKBAR_HEIGHT);

    DrawRect(Renderer, Rect, Ctx->Style.Colors[ui_COLOR_BUTTON]);
    if (Text)
        ui_DrawText(Renderer, Ctx, Text, Rect, ui_COLOR_TEXT, Opts);
}

typedef struct _app_state {
    platform_api Api;
    memory_arena Arena;
    renderer     Renderer;
    ui_context ui_Context;

    font_ RobotoMono;
} app_state;

external APP_INIT(Init) {
    app_state *s = (app_state *)p->Memory.Contents;

    s->Api   = SetPlatformApi(p);
    s->Arena = InitializeArena(p->Memory.Size - sizeof(app_state), (u8 *)p->Memory.Contents + sizeof(app_state));

    platform_api *Api      = &s->Api;
    memory_arena *Arena    = &s->Arena;
    renderer     *Renderer = &s->Renderer;

    s->ui_Context.Hover   =  0;
    s->ui_Context.Focus   =  0;
    s->ui_Context.LastId  =  0;
    s->ui_Context.Current = -1;

    FT_Library ft;
    FT_Init_FreeType(&ft);

    // LoadFont(Renderer, Api, "roboto.ttf", 0, 14);

    Renderer->Fonts[0] = LoadFontFreetype(Api, &ft, "roboto_mono.ttf");

    FT_Done_FreeType(ft);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    app_state *s = (app_state *)p->Memory.Contents;

    platform_api *Api      = &s->Api;
    memory_arena *Arena    = &s->Arena;
    renderer     *Renderer = &s->Renderer;

    /*s->ui_Context.MouseDown = p->mLeft || p->mRight || p->mMiddle;
    s->ui_Context.MouseWich = p->mLeft? ui_MOUSE_BUTTON_LEFT : 0;
    s->ui_Context.KeyDown = p->kBack || p->kReturn;
    s->ui_Context.KeyWich = p->kBack? ui_KEY_BACKSPACE : 0;
    s->ui_Context.MousePos = p->mPos;

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
    
    // DrawRect(Renderer, s->ui_Context.Layout.Body, GREY_800);

    ui_Label(Renderer, &s->ui_Context, "Button 1", 0);
    if (ui_Button(Renderer, &s->ui_Context, "BUTTON", 0))
        DrawRect(Renderer, rect_(0, 100, 10, 10), YELLOW_800);

    ui_NextRow(&s->ui_Context);

    ui_Label(Renderer, &s->ui_Context, "Button 2", 0);
    if (ui_Button(Renderer, &s->ui_Context, "BUTTON", 0))
        ui_Snackbar(Renderer, &s->ui_Context, "SOME TEXT", 0);

    ui_NextRow(&s->ui_Context);

    c8 Buff[32];
    ui_TextBox(Renderer, &s->ui_Context, Buff, 32, 0);

    texture Texture = s->RobotoMono.Atlas;
    rect Rect = rect_(0, 0, Texture.w, Texture.h);
    rv2 Pos = rv2_(0, 0);

    glBindTexture(GL_TEXTURE_2D, Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        rv2 TextureDim  = rv2_(Texture.w, Texture.h);

        glTexCoord2f(Rect.x / TextureDim.w, Rect.y / TextureDim.h);
        glVertex2f(Pos.x, Pos.y + Rect.h);

        glTexCoord2f((Rect.x + Rect.w) / TextureDim.w, Rect.y /TextureDim.h);
        glVertex2f(Pos.x + Rect.w, Pos.y + Rect.h);
        
        glTexCoord2f((Rect.x + Rect.w) / TextureDim.w, (Rect.y + Rect.h) / TextureDim.h);
        glVertex2f(Pos.x + Rect.w, Pos.y);

        glTexCoord2f(Rect.x / TextureDim.w, (Rect.y + Rect.h) / TextureDim.h);
        glVertex2f(Pos.x, Pos.y);
    } glEnd();
    glDisable(GL_TEXTURE_2D);*/

    DrawText(Renderer, "HgjIZW", 0, rv2_(100, 100), ORANGE_500);

    // Render(Renderer, p->WindowDim, s->ui_Context.Style.Colors[ui_COLOR_BACK]);
    Render(Renderer, p->WindowDim, GREY_900);
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