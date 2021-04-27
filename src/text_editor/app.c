#include "lingo.h"
#include "platform.h"
#include "maths.h"
#include "api.h"

platform_api GlobalPlatformApi;

#include "colors.h"
#include "app.h"

internal void DrawToken(renderer *Renderer, ui_ctx *Ctx, ui_input *Input, id Font, color Color, rv2 Pos) {

}

internal void DrawBuffer(editor_context *c, r32 y) {
    buffer   *Buffer   = c->Buffers[c->CurrentBuffer];
    ui_ctx   *Ctx      = c->uiCtx;
    ui_style *Style    = c->uiStyle;
    ui_input *Input    = c->uiInput;
    renderer *Renderer = c->Renderer;
    rv2 Pos = rv2_(Style->Padding.x,
                   Renderer->TargetClipRect.h - (Style->Padding.y*2 +
                   Renderer->Fonts[Style->Font].Ascender            +
                   Renderer->Fonts[Style->Font].Descender));
    u32 BufferLen = GetBufferLen(Buffer);
    font *Font = &Renderer->Fonts[Style->MonoFont];

    c8   Char;
    u32  Index;
    r32  TempX = Pos.x;
    rv2  Offset;
    rect GlyphRect;

    rect BufferRect = {0};
    BufferRect.x = Style->Padding.x;
    BufferRect.y = Style->Padding.y                      +
                   Renderer->Fonts[Style->Font].Ascender +
                   Renderer->Fonts[Style->Font].Descender;
    BufferRect.w = Renderer->TargetClipRect.w/2 - Style->Padding.x;
    BufferRect.h = Renderer->TargetClipRect.h - 2*BufferRect.y;

    DrawPushClip(Renderer, BufferRect);

    colorb Color = Style->DefaultTextColor;

    Pos.y += y;

    for (cursor Cursor = 0; Cursor < BufferLen; Cursor++) {
        Char   = GetBufferChar(Buffer, Cursor);
        Index  = (Char - 32 >= 0)? Char - 32 : '?' - 32;
        Offset = Font->GlyphOffsets[Index];
        GlyphRect = rect_(Pos.x + Offset.x, Pos.y - Offset.y, GetVecComps(Font->GlyphRects[Index].Dim));
        
        if (Input->mLeftButtonIsDown) {
            if (IsInsideRect(Input->mPos, GlyphRect) && IsInsideRect(Input->mPos, BufferRect)) {
                Buffer->Point = Cursor;
                c->nCurrentBufferLine   = GetBufferLine(Buffer, Buffer->Point);
                c->nCurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
            }
        }

        if (Cursor == Buffer->Point)
            c->CurrentCaretPos = rv2_(Pos.x +  Offset.x, Pos.y + Font->Descender);

        if (Char == ' ') {
            Pos.x += Font->GlyphAdvances[Index] + Style->CharSpacing;
            continue;
        }

        if (Char == '\r') {
            continue;
        }

        if (Char == '\n') {
            Pos.y -= Font->LineGap + Style->LineSpacing;
            Pos.x  = TempX;
            Color = Style->DefaultTextColor;
            continue;
        }

        if (Char == '/') {
            cursor Temp = Cursor;
            if (GetBufferChar(Buffer, ++Temp) == '/')
                Color = (colorb){GREEN_600};
        }

        DrawGlyph(Renderer, Style->MonoFont, Index, GlyphRect.Pos, Color);
    
        Pos.x += Font->GlyphAdvances[Index] + Style->CharSpacing;
    }

    // DrawRect(Renderer, Line,  (colorb){0x48484848/*GREY_800*/});
    // DrawRect(Renderer, Caret, Style->DefaultTextColor);
    DrawPopClip(Renderer);
}

global r32 y = 0;

void DrawUi(editor_context *c) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    r32 Ascender = c->Renderer->Fonts[c->uiStyle->MonoFont].Ascender;
    rect Line  = rect_(c->uiStyle->Padding.x, c->CurrentCaretPos.y, c->Renderer->TargetClipRect.w/2 - c->uiStyle->Padding.x, Ascender);
    rect Caret = rect_(c->CurrentCaretPos.x,  c->CurrentCaretPos.y, 2, Ascender);
    DrawRect(c->Renderer, Line,  (colorb){GREY_800});
    DrawRect(c->Renderer, Caret, (colorb){GREY_50 });
    y = uiSlder(c->uiCtx, c->uiStyle, c->uiInput, c->Renderer, y, rv2_(800, 100), 500);
    DrawBuffer(c, y*50000);
    c8 *Tabs[] = {
        "tab 1",
        "tab 2",
        "tab 3",
        "tab 4"
    };
    uiTabBar(c->uiCtx, c->uiStyle, c->uiInput, c->Renderer, Tabs, ArrayCount(Tabs));
    uiBottomBar(c->uiCtx, c->uiStyle, c->uiInput, c->Renderer, c->Filename,
                GetBufferLine(Buffer, Buffer->Point),
                GetBufferColumn(Buffer, Buffer->Point),
                c->dtFrame);
}

command Keymap[1024];

typedef struct _app_state {
    renderer Renderer;
    editor_context Context;
    memory_arena   Arena;
    i16 dLastMouseWheel;
} app_state;

external APP_INIT(Init) {
    //todo:
    //  do all these allocations from some memory pool
    //  because this is getting out of hand.
    Assert(sizeof(app_state) <= p->Memory.Size);
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

    State->Context.Renderer = &State->Renderer;

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

    State->Context.uiStyle->Font     = LoadFont(State->Context.Renderer, &GlobalPlatformApi, "roboto.ttf", 400, 24);
    State->Context.uiStyle->MonoFont = LoadFont(State->Context.Renderer, &GlobalPlatformApi, "roboto_mono.ttf", 400, 24);
    State->Context.uiStyle->IconFont = LoadFont(State->Context.Renderer, &GlobalPlatformApi, "icons.ttf", 400, 24);
    State->Context.uiStyle->Padding = rv2_(20, 20);
    State->Context.uiStyle->DefaultTextColor   = (colorb){GREY_50 };
    State->Context.uiStyle->ClickedButtonColor = (colorb){GREY_600};
    State->Context.uiStyle->HotButtonColor     = (colorb){GREY_700};
    State->Context.uiStyle->DefaultButtonColor = (colorb){GREY_800};
    State->Context.uiStyle->CharSpacing = 0;
    State->Context.uiStyle->LineSpacing = 0;
    State->Context.uiStyle->SliderHandleHeight = 20;
    State->Context.uiStyle->SliderHandleWidth  = 10;

    Keymap[KEY_NONE] = command_(cmd_proc_DoNothing, "DoNothing");
    Keymap[KEY_CHAR] = command_(cmd_proc_InsertChar, "InsertChar");
    Keymap[KEY_DEL]  = command_(cmd_proc_DeleteCharFoward, "DeleteCharFoward");
    Keymap[KEY_BACK] = command_(cmd_proc_DeleteCharBackward, "DeleteCharBackward");
    Keymap[KEY_TAB]  = command_(cmd_proc_Indent, "Indent");
    Keymap[KEY_LEFT]  = command_(cmd_proc_MoveCarretLeft, "MoveCarretLeft");
    Keymap[KEY_RIGHT] = command_(cmd_proc_MoveCarretRight, "MoveCarretRight");
    Keymap[KEY_CTRL | KEY_LEFT]  = command_(cmd_proc_MoveCarretToPrevToken, "MoveCarretToPrevToken");
    Keymap[KEY_CTRL | KEY_RIGHT] = command_(cmd_proc_MoveCarretToNextToken, "MoveCarretToNextToken");
    Keymap[KEY_UP]    = command_(cmd_proc_MoveCarretUp, "MoreCarretUp");
    Keymap[KEY_DOWN]  = command_(cmd_proc_MoveCarretDown, "MoveCarretDown");
    Keymap[KEY_HOME]  = command_(cmd_proc_MoveCarretToLineStart, "MoveCarretToLineStart");
    Keymap[KEY_END]   = command_(cmd_proc_MoveCarretToLineEnd, "MoveCarretToLineEnd");
    Keymap[KEY_CTRL | KEY_HOME]  = command_(cmd_proc_MoveCarretToBufferStart, "MoveCarretToLineStart");
    Keymap[KEY_CTRL | KEY_END]   = command_(cmd_proc_MoveCarretToBufferEnd,   "MoveCarretToLineEnd");
    Keymap[KEY_RETURN] = command_(cmd_proc_InsertNewLine, "InsertNewLine");
    Keymap[KEY_CTRL | 'S'] = command_(cmd_proc_SaveFile, "SaveFile");
    Keymap[KEY_CTRL | 'O'] = command_(cmd_proc_OpenFile, "OpenFile");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

global u8 KeyAccumulator = 0; //note: hack!

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    State->Context.Filename = "a.c";

    UpdateEditorContextInput(&State->Context, p);

    if (Keymap[State->Context.LastKeyComb].Proc)
        Keymap[State->Context.LastKeyComb].Proc(&State->Context);

    DrawUi(&State->Context);
    Render(&State->Renderer, p->WindowDim, (colorb){GREY_900});
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

    Keymap[KEY_NONE] = command_(cmd_proc_DoNothing, "DoNothing");
    Keymap[KEY_CHAR] = command_(cmd_proc_InsertChar, "InsertChar");
    Keymap[KEY_DEL]  = command_(cmd_proc_DeleteCharFoward, "DeleteCharFoward");
    Keymap[KEY_BACK] = command_(cmd_proc_DeleteCharBackward, "DeleteCharBackward");
    Keymap[KEY_TAB]  = command_(cmd_proc_Indent, "Indent");
    Keymap[KEY_LEFT]  = command_(cmd_proc_MoveCarretLeft, "MoveCarretLeft");
    Keymap[KEY_RIGHT] = command_(cmd_proc_MoveCarretRight, "MoveCarretRight");
    Keymap[KEY_CTRL | KEY_LEFT]  = command_(cmd_proc_MoveCarretToPrevToken, "MoveCarretToPrevToken");
    Keymap[KEY_CTRL | KEY_RIGHT] = command_(cmd_proc_MoveCarretToNextToken, "MoveCarretToNextToken");
    Keymap[KEY_UP]    = command_(cmd_proc_MoveCarretUp, "MoreCarretUp");
    Keymap[KEY_DOWN]  = command_(cmd_proc_MoveCarretDown, "MoveCarretDown");
    Keymap[KEY_HOME]  = command_(cmd_proc_MoveCarretToLineStart, "MoveCarretToLineStart");
    Keymap[KEY_END]   = command_(cmd_proc_MoveCarretToLineEnd, "MoveCarretToLineEnd");
    Keymap[KEY_CTRL | KEY_HOME]  = command_(cmd_proc_MoveCarretToBufferStart, "MoveCarretToLineStart");
    Keymap[KEY_CTRL | KEY_END]   = command_(cmd_proc_MoveCarretToBufferEnd,   "MoveCarretToLineEnd");
    Keymap[KEY_RETURN] = command_(cmd_proc_InsertNewLine, "InsertNewLine");
    Keymap[KEY_CTRL | 'S'] = command_(cmd_proc_SaveFile, "SaveFile");
    Keymap[KEY_CTRL | 'O'] = command_(cmd_proc_OpenFile, "OpenFile");
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}

#if 0
#include "lingo.h"

//note: will I ever use this?

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
    } else {
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
