#include "lingo.h"
#include "platform.h"
#include "maths.h"
#include "api.h"

platform_api GlobalPlatformApi;

#include "app.h"

internal void DrawBuffer(editor_context *c) {
    buffer   *Buffer   = c->Buffers[c->CurrentBuffer];
    ui_ctx   *Ctx      = c->uiCtx;
    ui_style *Style    = c->uiStyle;
    ui_input *Input    = c->uiInput;
    renderer *Renderer = Ctx->Renderer;
    rv2 Pos = rv2_(Style->Padding.x, Renderer->TargetClipRect.h - 100);
    u32 BufferLen = GetBufferLen(Buffer);
    font *Font = &Renderer->Fonts[Style->MonoFont];

    c8   Char;
    u32  Index;
    r32  TempX = Pos.x;
    rv2  Offset;
    rect GlyphRect;
    rect Caret = rect_(0, 0, 0, 0);

    rect BufferRect = {0};
    BufferRect.x = Style->Padding.x;
    BufferRect.y = 100;
    BufferRect.w = Renderer->TargetClipRect.w/2 - Style->Padding.x;
    BufferRect.h = Renderer->TargetClipRect.h - 100;

    DrawPushClip(Renderer, BufferRect);

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
            Caret = rect_(Pos.x, Pos.y + Font->Descender, 2, Font->Ascender);

        if (Char == ' ') {
            Pos.x += Font->GlyphAdvances[Index] + Style->CharSpacing;
            continue;
        }

        if (Char == '\r') {
            continue;
        }

        if (Char == '\n') {
            if (Cursor == Buffer->Point)
                Caret = rect_(Pos.x +  Offset.x, Pos.y - Offset.y + Font->Descender, 2, Font->Ascender);
            Pos.y -= Font->LineGap + Style->LineSpacing;
            Pos.x  = TempX;
            continue;
        }

        DrawGlyph(Renderer, Style->MonoFont, Index, GlyphRect.Pos, Style->DefaultTextColor);
    
        Pos.x += Font->GlyphAdvances[Index] + Style->CharSpacing;
    }

    DrawRect(Renderer, Caret, Style->DefaultTextColor);
    DrawPopClip(Renderer);
}

void DrawUi(editor_context *c) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    uiBottomBar(c->uiCtx, c->uiStyle, c->uiInput, c->Filename,
                GetBufferLine(Buffer, Buffer->Point),
                GetBufferColumn(Buffer, Buffer->Point),
                c->dtFrame);
    
    DrawBuffer(c);
}

typedef struct _app_state {
    renderer Renderer;
    command  Keymap[1024];
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

    State->Arena = InitializeArena(p->Memory.Size     - sizeof(app_state),
                             (u8 *)p->Memory.Contents + sizeof(app_state));

    State->Context.uiCtx   = PushToArena(&State->Arena, sizeof(ui_ctx));
    State->Context.uiStyle = PushToArena(&State->Arena, sizeof(ui_style));
    State->Context.uiInput = PushToArena(&State->Arena, sizeof(ui_input));
    State->Context.Buffers = PushToArena(&State->Arena, sizeof(buffer *)*2);
    State->Context.Buffers[0] = CreateBuffer(8);
    
    State->Context.uiCtx->Renderer = &State->Renderer;
    State->Context.uiCtx->Hot      = -1;
    State->Context.uiCtx->Clicked  = -1;
    State->Context.uiCtx->Last     = -1;
    State->Context.uiCtx->Current  = -1;
    State->Context.uiCtx->NoItems  = 256;

    State->Context.uiStyle->Font     = LoadFont(State->Context.uiCtx->Renderer, &PlatformApi, "roboto.ttf", 400, 24);
    State->Context.uiStyle->MonoFont = LoadFont(State->Context.uiCtx->Renderer, &PlatformApi, "roboto_mono.ttf", 400, 24);
    State->Context.uiStyle->IconFont = LoadFont(State->Context.uiCtx->Renderer, &PlatformApi, "icons.ttf", 400, 24);
    State->Context.uiStyle->Padding = rv2_(20, 20);
    State->Context.uiStyle->DefaultTextColor   = (colorb){0xFAFAFAFF};
    State->Context.uiStyle->HotButtonColor     = (colorb){0x606060FF};
    State->Context.uiStyle->ClickedButtonColor = (colorb){0x808080FF};
    State->Context.uiStyle->DefaultButtonColor = (colorb){0x404040FF};
    State->Context.uiStyle->CharSpacing = 0;
    State->Context.uiStyle->LineSpacing = 0;
    State->Context.uiStyle->SliderHandleHeight = 20;
    State->Context.uiStyle->SliderHandleWidth  = 10;

    State->Keymap[KEY_NONE] = command_(cmd_proc_DoNothing, "DoNothing");
    State->Keymap[KEY_CHAR] = command_(cmd_proc_InsertChar, "InsertChar");
    State->Keymap[KEY_DEL]  = command_(cmd_proc_DeleteCharFoward, "DeleteCharFoward");
    State->Keymap[KEY_BACK] = command_(cmd_proc_DeleteCharBackward, "DeleteCharBackward");
    State->Keymap[KEY_TAB]  = command_(cmd_proc_Indent, "Indent");
    State->Keymap[KEY_LEFT]  = command_(cmd_proc_MoveCarretLeft, "MoveCarretLeft");
    State->Keymap[KEY_RIGHT] = command_(cmd_proc_MoveCarretRight, "MoveCarretRight");
    State->Keymap[KEY_CTRL | KEY_LEFT]  = command_(cmd_proc_MoveCarretToPrevToken, "MoveCarretToPrevToken");
    State->Keymap[KEY_CTRL | KEY_RIGHT] = command_(cmd_proc_MoveCarretToNextToken, "MoveCarretToNextToken");
    State->Keymap[KEY_UP]    = command_(cmd_proc_MoveCarretUp, "MoreCarretUp");
    State->Keymap[KEY_DOWN]  = command_(cmd_proc_MoveCarretDown, "MoveCarretDown");
    State->Keymap[KEY_HOME]  = command_(cmd_proc_MoveCarretToLineStart, "MoveCarretToLineStart");
    State->Keymap[KEY_END]   = command_(cmd_proc_MoveCarretToLineEnd, "MoveCarretToLineEnd");
    State->Keymap[KEY_CTRL | KEY_HOME]  = command_(cmd_proc_MoveCarretToBufferStart, "MoveCarretToLineStart");
    State->Keymap[KEY_CTRL | KEY_END]   = command_(cmd_proc_MoveCarretToBufferEnd,   "MoveCarretToLineEnd");
    State->Keymap[KEY_RETURN] = command_(cmd_proc_InsertNewLine, "InsertNewLine");
    State->Keymap[KEY_CTRL | 'S'] = command_(cmd_proc_SaveFile, "SaveFile");
    State->Keymap[KEY_CTRL | 'O'] = command_(cmd_proc_OpenFile, "OpenFile");    

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    State->Context.Filename = "a.c";

    UpdateEditorContextInput(&State->Context, p);

    if (State->Keymap[State->Context.LastKeyComb].Proc)
        State->Keymap[State->Context.LastKeyComb].Proc(&State->Context);

    DrawUi(&State->Context);
    Render(&State->Renderer, p->WindowDim, (colorb){0x202020FF});
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;

    // Init(p);
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
