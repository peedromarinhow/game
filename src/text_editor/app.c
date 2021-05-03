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

u8 CHAR_TO_DIGIT[128] = {
    ['0'] = 0,
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['a'] = 10,
    ['A'] = 10,
    ['b'] = 11,
    ['B'] = 11,
    ['c'] = 12,
    ['C'] = 12,
    ['d'] = 13,
    ['D'] = 13,
    ['e'] = 14,
    ['E'] = 14,
    ['f'] = 15,
    ['F'] = 15,
};

typedef enum _token_type {
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_DELIMITER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_STR,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_NAME
} token_type;

typedef enum _token_mode {
    TOKEN_MODE_NONE,
    TOKEN_MODE_HEX,
    TOKEN_MODE_OCT,
    TOKEN_MODE_BIN,
    TOKEN_MODE_CHAR
} token_mode;

typedef struct _token {
    token_type Type;
    token_mode Mode;

    u32 Error;

    c8 *Start;
    c8 *End;

    union {
        u64 iVal;
        f64 fVal;
        c8 *sVal;
        c8 *Name;
    };
    u32 NameLen;
} token;

internal c8 *ScanInt(c8 *Stream, token *Token) {
    Token->Start = Stream;
    u64 Base = 10;
    if (*Stream == '0') {
        Stream++;
        if (tolower(*Stream) == 'x') {
            Stream++;
            Token->Mode = TOKEN_MODE_HEX;
            Base = 16;
        }
        else
        if (tolower(*Stream) == 'b') {
            Stream++;
            Token->Mode = TOKEN_MODE_BIN;
            Base = 2;
        }
        else
        if (isdigit(*Stream)) {
            Token->Mode = TOKEN_MODE_OCT;
            Base = 8;
        }
    }
    u64 Val = 0;
    while (1) {
        u64 Digit = CHAR_TO_DIGIT[*Stream];
        if (Digit == 0 && *Stream != '0') {
            break;
        }
        if (Digit > Base) {
            //todo: switch color to some error color or use those squiggly undelines idk
            Token->Error = 1;
        }
        if (Val > (UINT64_MAX - Digit)/Base) {
            //note: see above todo
            Token->Error = 1;
            while (isdigit(*Stream))
                Stream++;
            Val = 0;
        }
        Val = Val * Base + Digit;
        Stream++;
    }
    Token->End  = Stream;
    Token->iVal = Val;
    Token->Type = TOKEN_TYPE_INT;

    return Stream;
}

internal c8 *ScanFloat(c8 *Stream, token *Token) {
    c8 *Start    = Stream;
    Token->Start = Start;
    while (isdigit(*Stream)) {
        Stream++;
    }
    if (*Stream == '.') {
        Stream++;
    }
    Stream++;
    while (isdigit(*Stream)) {
        Stream++;
    }
    if (tolower(*Stream) == 'e') {
        Stream++;
        if (*Stream == '+' || *Stream == '-') {
            Stream++;
        }
        if (!isdigit(*Stream)) {
            Token->Error = 1;
        }
        while (isdigit(*Stream)) {
            Stream++;
        }
    }
    c8 *End = Stream;
    f64 Val = strtod(Start, NULL);
    if (Val == HUGE_VAL || Val == -HUGE_VAL) {
        Token->Error = 1;
    }
    Token->End  = Stream;
    Token->fVal = Val;
    Token->Type = TOKEN_TYPE_FLOAT;

    return Stream;
}

char ESCAPE_TO_CHAR[128] = {
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['b'] = '\b',
    ['a'] = '\a',
    ['0'] =   0,
};

internal c8 *ScanStr(c8 *Stream, token *Token) {
    Assert(*Stream == '"');
    Token->Start = Stream;
    Stream++;
    char *Str = NULL;
    while (*Stream && *Stream != '"') {
        char Val = *Stream;
        if (Val == '\n') {
            Token->Error = 1;
        }
        else
        if (Val == '\\') {
            Stream++;
            Val = ESCAPE_TO_CHAR[*Stream];
            if (Val == 0 && *Stream != '0') {
                Token->Error = 1;
            }
        }
        Stream++;
    }
    if (*Stream) {
        Assert(*Stream == '"');
        Stream++;
    }
    else {
        Token->Error = 1;
    }
    Token->End  = Stream;
    Token->sVal = Str;
    Token->Type = TOKEN_TYPE_STR;

    return Stream;
}

internal c8 *ScanChar(c8 *Stream, token *Token) {
    Token->Start = Stream;
    Stream++;
    c8 Val = 0;
    if (*Stream == '\'') {
        //
        Stream++;
    }
    else
    if (*Stream == '\n') {
        //
    }
    else
    if (*Stream == '\\') {
        Stream++;
        Val = ESCAPE_TO_CHAR[*Stream];
        if (Val == 0 && *Stream != '0') {
            //
        }
        Stream++;
    }
    else {
        Val = *Stream;
        Stream++;
    }
    
    if (*Stream != '\'') {
        //
    }
    else {
        Stream++;
    }
    Token->End  = Stream;
    Token->iVal = Val;
    Token->Type = TOKEN_TYPE_INT;
    Token->Mode = TOKEN_MODE_CHAR;

    return Stream;
}

#define CASE1(c, c1, k)             \
    case c: {                       \
        Token->Type = *Stream++;    \
        if (*Stream == c1) {        \
            Token->Type = k;        \
            Stream++;               \
        }                           \
        break;                      \
    }

#define CASE2(c, c1, k, c2, k1)     \
    case c: {                       \
        Token->Type = *Stream++;    \
        if (*Stream == c1) {        \
            Token->Type = k;        \
            Stream++;               \
        }                           \
        else                        \
        if (*Stream == c2) {        \
            Token->Type = k1;       \
            Stream++;               \
        }                           \
        break;                      \
    }

i32 IsKeyword(c8 *Name) {
    return 0;
}

internal c8 *GetSubStr(c8 *Start, c8 *End) {
    u32 Len = (i32)(End - Start);
    c8 *Str = GlobalPlatformApi.AllocateMemory(Len + 1);
    CopyMemory(Str, Start, Len);
    Str[Len] = 0;
    return Str;
}

internal c8 *NextToken(c8 *Stream, token *Token) {
TOP:
    Token->Start = Stream;
    Token->Mode  = 0;
    switch (*Stream) {
        case ' ': case '\n': case '\r': case '\t': case '\v': {
            while (isspace(*Stream)) {
                Stream++;
            }
            goto TOP;
            break;
        }
        case '\'': {
            Stream = ScanChar(Stream, Token);
            break;
        }
        case '"': {
            Stream = ScanStr(Stream, Token);
            break;
        }
        case '.': {
            Stream = ScanFloat(Stream, Token);
            break;
        }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            while (isdigit(*Stream)) {
                Stream++;
            }
            c8 c = *Stream;
            Stream = Token->Start;
            if (c == '.' || tolower(c) == 'e') {
                Stream = ScanFloat(Stream, Token);
            }
            else {
                Stream = ScanInt(Stream, Token);
            }
            break;
        }
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y':
        case 'z':
        case '_': {
            c8 *Start = Stream++;
            while (isalnum(*Stream) || *Stream == '_') {
                Stream++;
            }
            Token->Name    = Token->Start;
            Token->NameLen = Stream - Token->Start;
            Token->Type = IsKeyword(Token->Name) ? TOKEN_TYPE_KEYWORD : TOKEN_TYPE_NAME;
            break;
        }
        case '<': {
            Token->Type = *Stream++;
            if (*Stream == '<') {
                Token->Type = TOKEN_TYPE_OPERATOR;//TOKEN_TYPE_LSHIFT;
                Stream++;
                if (*Stream == '=') {
                    Token->Type = TOKEN_TYPE_OPERATOR;//TOKEN_TYPE_LSHIFT_ASSIGN;
                    Stream++;
                }
            }
            else
            if (*Stream == '=') {
                Token->Type = TOKEN_TYPE_OPERATOR;//TOKEN_TYPE_LTEQ;
                Stream++;
            }
            break;
        }
        case '>': {
            Token->Type = *Stream++;
            if (*Stream == '>') {
                Token->Type = TOKEN_TYPE_OPERATOR;//TOKEN_TYPE_RSHIFT;
                Stream++;
                if (*Stream == '=') {
                    Token->Type = TOKEN_TYPE_OPERATOR;//TOKEN_TYPE_RSHIFT_ASSIGN;
                    Stream++;
                }
            }
            else
            if (*Stream == '=') {
                Token->Type = TOKEN_TYPE_OPERATOR;//TOKEN_TYPE_GTEQ;
                Stream++;
            }
            break;
        }
        CASE1('&', '=', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_AND_ASSIGN*/)
        CASE1('|', '=', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_OR_ASSIGN*/)
        CASE1('^', '=', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_XOR_ASSIGN*/)
        CASE1('*', '=', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_MUL_ASSIGN*/)
        CASE1('/', '=', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_DIV_ASSIGN*/)
        CASE1('%', '=', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_MOD_ASSIGN*/)
        CASE1('=', '=', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_EQ*/)
        CASE1('!', '=', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_NOTEQ*/)
        CASE2('+', '=', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_ADD_ASSIGN*/, '+', TOKEN_TYPE_OPERATOR/*TOKEN_TYPE_INC*/)
        case '-': {
            Token->Type = *Stream++;
            if (*Stream == '=') {
                Token->Type = TOKEN_TYPE_OPERATOR;//TOKEN_TYPE_MIN_ASSIGN;
                Stream++;
                if (*Stream == '-') {
                    Token->Type = TOKEN_TYPE_OPERATOR;//TOKEN_TYPE_DEC;
                    Stream++;
                }
            }
            else
            if (*Stream == '>') {
                Token->Type = TOKEN_TYPE_OPERATOR;//TOKEN_TYPE_ARR;
                Stream++;
            }
            break;
        }
        default: {
            Token->Type = *Stream++;
            break;
        }
    }
    Token->End = Stream;
    return Stream;
}

internal void DrawScannedBuffer(editor_context *c, c8 *Stream) {
    ui_ctx   *Ctx      = c->uiCtx;
    ui_style *Style    = c->uiStyle;
    ui_input *Input    = c->uiInput;
    renderer *Renderer = c->Renderer;

    font *Font = &Renderer->Fonts[Style->MonoFont];

    rv2 Pos = rv2_(Style->Padding.x, Renderer->TargetClipRect.h - (Style->Padding.y*2 + Renderer->Fonts[Style->MainFont].Height));
    r32 TempX = Pos.x;

    token Token = {0};

    u32 Index      =  0;
    rv2 Offset     = {0};
    rect GlyphRect = {0};

    colorb Color = GREY_50;

    Stream = NextToken(Stream, &Token);
    while (Token.Type != TOKEN_TYPE_EOF) {

        if (Token.Type == TOKEN_TYPE_EOF)
            Color = ORANGE_600;
        if (Token.Type == TOKEN_TYPE_OPERATOR)
            Color = GREY_600;
        if (Token.Type == TOKEN_TYPE_DELIMITER)
            Color = GREY_600;
        if (Token.Type == TOKEN_TYPE_KEYWORD)
            Color = BLUE_600;
        if (Token.Type == TOKEN_TYPE_STR)
            Color = GREEN_600;
        if (Token.Type == TOKEN_TYPE_INT)
            Color = RED_600;
        if (Token.Type == TOKEN_TYPE_FLOAT)
            Color = RED_600;
        if (Token.Type == TOKEN_TYPE_NAME)
            Color = BLUE_600;

        for (c8 *Char = Token.Start; Char < Token.End; Char++) {
            Index  = (*Char - 32 >= 0)? *Char - 32 : '?' - 32;
            Offset = Font->GlyphOffsets[Index];
            GlyphRect = rect_(Pos.x + Offset.x, Pos.y - Offset.y, GetVecComps(Font->GlyphRects[Index].Dim));

            DrawGlyph(Renderer, Style->MonoFont, Index, GlyphRect.Pos, Color);
            Pos.x += Font->GlyphAdvances[Index];
        }
        Stream = NextToken(Stream, &Token);
    }
}

internal void DrawBottomBar(editor_context *c) {
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
    sprintf_s(TextBuffer, 32, "%u, %u", c->nCurrentBufferLine, c->nCurrentBufferColumn);
    uiButton(Ctx, Style, Input, Renderer, rv2_(x, 0), TextBuffer);
    x = Renderer->TargetClipRect.w - MeasureText(Renderer, "0.000000", Style->MainFont, rv2_(0, 0)).w - Style->Padding.x*2;
    sprintf_s(TextBuffer, 32, "%f", c->dtFrame);
    uiButton(Ctx, Style, Input, Renderer, rv2_(x, 0), TextBuffer);
}

internal u32 uiTabBar(editor_context *c, c8 **Tabs, u32 NoTabs) {
    buffer   *Buffer   = c->Buffers[c->CurrentBuffer];
    ui_ctx   *Ctx      = c->uiCtx;
    ui_style *Style    = c->uiStyle;
    ui_input *Input    = c->uiInput;
    renderer *Renderer = c->Renderer;
    colorb    BackgroundColor = Style->DefaultColor;

    rect BarBackground;
    BarBackground.w = Renderer->TargetClipRect.w;
    BarBackground.h = Style->Padding.y                      +
                      Renderer->Fonts[Style->MainFont].Ascender +
                      Renderer->Fonts[Style->MainFont].Descender;
    BarBackground.Pos = rv2_(0, Renderer->TargetClipRect.h - BarBackground.h);
    DrawRect(Renderer, BarBackground, BackgroundColor);

    u32 ClickedTab = -1;

    r32 x = 0;
    for (u32 TabIndex = 0; TabIndex < NoTabs; TabIndex++) {
        if (uiButton(Ctx, Style, Input, Renderer, rv2_(x, Renderer->TargetClipRect.h - BarBackground.h), Tabs[TabIndex]))
            ClickedTab = TabIndex;
        x += MeasureText(Renderer, Tabs[TabIndex], Style->MainFont, rv2_(0, 0)).w + Style->Padding.x;
    }

    return ClickedTab;
}

internal void DrawBuffer(editor_context *c) {
    buffer   *Buffer   = c->Buffers[c->CurrentBuffer];
    ui_ctx   *Ctx      = c->uiCtx;
    ui_style *Style    = c->uiStyle;
    ui_input *Input    = c->uiInput;
    renderer *Renderer = c->Renderer;
    rv2   Pos  = rv2_(Style->Padding.x, Renderer->TargetClipRect.h - (Style->Padding.y*2 + Renderer->Fonts[Style->MainFont].Height));
    font *Font = &Renderer->Fonts[Style->MonoFont];

    c8   Char;
    u32  Index;
    r32  TempX = Pos.x;
    rv2  Offset;
    rect GlyphRect;

    rect BufferRect = {0};
    BufferRect.x = Style->Padding.x;
    BufferRect.y = Style->Padding.y + Renderer->Fonts[Style->MainFont].Height;
    BufferRect.w = Renderer->TargetClipRect.w/2 - Style->Padding.x;
    BufferRect.h = Renderer->TargetClipRect.h*2 - 2*BufferRect.y;

    colorb Color = GREY_50;

    Pos.y;

    r32  Ascender = c->Renderer->Fonts[c->uiStyle->MonoFont].Ascender;
    rect Line  = rect_(c->uiStyle->Padding.x, c->CurrentCaretPos.y, c->Renderer->TargetClipRect.w/2 - c->uiStyle->Padding.x, Ascender);
    rect Caret = rect_(c->CurrentCaretPos.x,  c->CurrentCaretPos.y, 2, Ascender);
    DrawRect(c->Renderer, Line,  GREY_800);
    DrawRect(c->Renderer, Caret, GREY_50);

    DrawPushClip(Renderer, BufferRect);

    u32 BufferLen = GetBufferLen(Buffer);
    for (cursor Cursor = 0; Cursor < BufferLen; Cursor++) {
        Char   = GetBufferChar(Buffer, Cursor);
        Index  = (Char - 32 >= 0)? Char - 32 : '?' - 32;
        Offset = Font->GlyphOffsets[Index];
        GlyphRect = rect_(Pos.x + Offset.x, Pos.y - Offset.y, GetVecComps(Font->GlyphRects[Index].Dim));
        
        if (Input->mLeft) {
            if (IsInsideRect(Input->mPos, GlyphRect) && IsInsideRect(Input->mPos, BufferRect)) {
                Buffer->Point = Cursor;
                c->nCurrentBufferLine   = GetBufferLine(Buffer, Buffer->Point);
                c->nCurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
            }
        }

        if (Cursor == Buffer->Point)
            c->CurrentCaretPos = rv2_(Pos.x +  Offset.x, Pos.y + Font->Descender);

        if (Char == ' ') {
            Pos.x += Font->GlyphAdvances[Index];// + Style->CharSpacing;
            continue;
        }

        if (Char == '\r') {
            continue;
        }

        if (Char == '\n') {
            Pos.y -= Font->LineGap;// + Style->LineSpacing;
            Pos.x  = TempX;
            Color  = GREY_50;
            continue;
        }

        if (Char == '/') {
            cursor Temp = Cursor;
            if (GetBufferChar(Buffer, ++Temp) == '/')
                Color = GREEN_600;
        }

        DrawGlyph(Renderer, Style->MonoFont, Index, GlyphRect.Pos, Color);
    
        Pos.x += Font->GlyphAdvances[Index];// + Style->CharSpacing;
    }
    DrawPopClip(Renderer);
}

global r32 y = 0;

internal void DrawUi(editor_context *c, c8 *Stream) {
    // DrawBuffer(c);
    DrawScannedBuffer(c, Stream);
    DrawBottomBar(c);

    // file_group Tabs = GlobalPlatformApi.GetDirFilenames("D:/code/etc/etcetc/*.c");
    // c8 PathBuffer[256];
    // for (c8 *Char = PathBuffer; *Char; Char++)
    //     *Char = '\0';
    // if (uiTabBar(c->uiCtx, c->uiStyle, c->uiInput, c->Renderer, Tabs.Filenames, Tabs.NoFiles) != -1)
    //     c->Tab = uiTabBar(c->uiCtx, c->uiStyle, c->uiInput, c->Renderer, Tabs.Filenames, Tabs.NoFiles);
    // if (c->Tab != -1) {
    //     sprintf_s(PathBuffer, 256, "D:/code/etc/etcetc/%s", Tabs.Filenames[c->Tab]);
    //     c->Filename = PathBuffer;
}

command Keymap[1024];

internal void SetKeymap() {
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

internal void SetApi(platform *p) {
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

    c8 *HackLexTextStream;
    token HackLexTextToken;
} app_state;

external APP_INIT(Init) {
    Assert(sizeof(app_state) <= p->Memory.Size);
    app_state *State = (app_state *)p->Memory.Contents;

    SetApi(p);
    SetKeymap();

    State->Context.Renderer = &State->Renderer;
    State->Context.Filename = "a.c";

    State->Arena = InitializeArena(p->Memory.Size     - sizeof(app_state),
                             (u8 *)p->Memory.Contents + sizeof(app_state));

    State->Context.uiCtx   = PushToArena(&State->Arena, sizeof(ui_ctx));
    State->Context.uiStyle = PushToArena(&State->Arena, sizeof(ui_style));
    State->Context.uiInput = PushToArena(&State->Arena, sizeof(ui_input));
    State->Context.Buffers = PushToArena(&State->Arena, sizeof(buffer *)*2);
    State->Context.Buffers[0] = CreateBuffer(8);
    State->Context.CurrentCaretPos = rv2_(0, -100); //note: hack! (?: this may actually be reasonable)

    State->Context.uiCtx->Hot      = -1;
    State->Context.uiCtx->Clicked  = -1;
    State->Context.uiCtx->Last     = -1;
    State->Context.uiCtx->Current  = -1;
    State->Context.uiCtx->NoItems  = 256;

    State->Context.uiStyle->MainFont = LoadFont(State->Context.Renderer, &GlobalPlatformApi, "roboto.ttf", 400, 24);
    State->Context.uiStyle->MonoFont = LoadFont(State->Context.Renderer, &GlobalPlatformApi, "roboto_mono.ttf", 400, 24);
    State->Context.uiStyle->IconFont = LoadFont(State->Context.Renderer, &GlobalPlatformApi, "icons.ttf", 400, 24);
    State->Context.uiStyle->Padding  = rv2_(10, 10);
    State->Context.uiStyle->ClickedColor = GREY_600;
    State->Context.uiStyle->HotColor     = GREY_700;
    State->Context.uiStyle->DefaultColor = GREY_800;

    State->HackLexTextStream = GlobalPlatformApi.LoadFile("a.c").Data;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

global u8 KeyAccumulator = 0; //note: hack!

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    UpdateEditorContextInput(&State->Context, p);
    DrawUi(&State->Context, State->HackLexTextStream);
    if (Keymap[State->Context.LastKeyComb].Proc)
        Keymap[State->Context.LastKeyComb].Proc(&State->Context);
    Render(&State->Renderer, p->WindowDim, GREY_900);
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;

    State->Context.Filename = "a.c";

    SetApi(p);
    SetKeymap();
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}

#if 0
#include "lingo.h"

//note: will this ever be used?

//note:
//  Stretchy buffers.
//  Invented by Sean Barret?
//  Interface:
//    i32 *Buffer = NULL;
//    BufferPush(Buffer, 42);
//    BufferPush(Buffer, 21);
//    for (i32 i = 0, i < BufferLen(Buffer), i++) {...}
//

typedef struct _buffer_header {
    u32 Len;
    u32 Capacity;
    c8 *Buffer;
} buffer_header;

// annotation
#define BUFF(x) x

/* internal functions
    */
#define _BufferHeader(Buffer) ((buffer_header *)((c8 *)Buffer - offsetof(buffer_header, Buffer)))
#define _BufferFits(Buffer, n) (GetBuffeLen(Buffer) + (n) <= GetBufferCapacity(Buffer))
#define _BufferFit(Buffer, n) (_BufferFits(Buffer, n) ? 0 : ((Buffer) = _GrowBuffer((Buffer), GetBuffeLen(Buffer) + (n), sizeof(*(Buffer)))))

/* public functions
    */
#define GetBuffeLen(Buffer) ((Buffer) ? _BufferHeader(Buffer)->Len : 0)
#define GetBufferCapacity(Buffer) ((Buffer) ? _BufferHeader(Buffer)->Capacity : 0)
#define GetBufferEnd(Buffer) ((Buffer) + GetBuffeLen(Buffer))
#define FreeBuffer(Buffer) ((Buffer) ? (free(_BufferHeader(Buffer)), (Buffer) = NULL) : 0)
#define PushToBuffer(Buffer, ...) (_BufferFit((Buffer), 1), (Buffer)[_BufferHeader(Buffer)->Len++] = (__VA_ARGS__))
#define PrintfBuffer(Buffer, ...) ((Buffer) = _PrintfBuffer((Buffer), __VA_ARGS__))
#define ClearBuffer(Buffer) ((Buffer) ? _BufferHeader(Buffer)->Len = 0 : 0)

internal void *_GrowBuffer(const c8 *Buffer, u32 NewLen, u32 ElementSize) {
    Assert(GetBufferCapacity(Buffer) <= (SIZE_MAX - 1)/2);
    u32 NewCapacity = MAX(1 + 2 * GetBufferCapacity(Buffer), NewLen);
    Assert(NewLen <= NewCapacity);
    Assert(NewCapacity <= (SIZE_MAX - offsetof(buffer_header, Buffer))/ElementSize);
    u32 NewSize = offsetof(buffer_header, Buffer) + NewCapacity * ElementSize;

    buffer_header *NewHeader;
    if (Buffer) {
        NewHeader = xrealloc(_BufferHeader(Buffer), NewSize);
    }
    else {
        NewHeader = xmalloc(NewSize);
        NewHeader->Len = 0;
    }
    NewHeader->Capacity = NewCapacity;
    return NewHeader->Buffer;
}

internal c8 *_PrintfBuffer(c8 *Buffer, const c8 *Format, ...) {
    va_list Args;
    va_start(Args, Format);
    u32 n = vsnprintf(NULL, 0, Format, Args);
    va_end(Args);
    if (GetBuffeLen(Buffer) == 0) {
        n++;
    }
    _BufferFit(Buffer, n + GetBuffeLen(Buffer));
    c8 *Dest = GetBuffeLen(Buffer) == 0 ? Buffer : Buffer + GetBuffeLen(Buffer) - 1;
    va_start(Args, Format);
    vsnprintf(Dest, Buffer + GetBufferCapacity(Buffer) - Dest, Format, Args);
    va_end(Args);
    _BufferHeader(Buffer)->Len += n;
    return Buffer;
}
#endif
