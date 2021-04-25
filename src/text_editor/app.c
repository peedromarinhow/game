#include "lingo.h"
#include "platform.h"
#include "maths.h"
#include "api.h"

platform_api GlobalPlatformApi;

#include "renderer.h"
#include "ui.h"

// #include "text_buffer.h"

typedef u32 cursor;

typedef struct _buffer {
    c8 *Data;
    id  Id;

    cursor GapStart;
    cursor GapEnd;
    cursor Point;
    u32    End;
} buffer;

internal finginline u32 GetBufferGapSize(buffer *Buffer) {
    return Buffer->GapEnd - Buffer->GapStart;
}

internal finginline u32 GetBufferLen(buffer *Buffer) {
    return Buffer->End - GetBufferGapSize(Buffer);
}

internal finginline u32 GetCursorIndex(buffer *Buffer, u32 Cursor) {
    return (Cursor < Buffer->GapStart)? Cursor : Cursor + GetBufferGapSize(Buffer);
}

internal finginline void AssertBufferInvariants(buffer *Buffer) {
    Assert(Buffer->Data);
    Assert(Buffer->GapStart <= Buffer->GapEnd);
    Assert(Buffer->GapEnd <= Buffer->End);
}

internal finginline void AssertCursorInvariants(buffer *Buffer, u32 Cursor) {
    Assert(Cursor <= GetBufferLen(Buffer));
}

internal finginline c8 GetBufferChar(buffer *Buffer, u32 Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    return Buffer->Data[GetCursorIndex(Buffer, Cursor)];
}

internal finginline void SetBufferChar(buffer *Buffer, u32 Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    Buffer->Data[GetCursorIndex(Buffer, Cursor)] = Char;
}

internal buffer *CreateBuffer(u32 InitialGapSize, c8 *Filename) {
    buffer *Buffer = GlobalPlatformApi.AllocateMemory(sizeof(buffer)); {
        Buffer->Data     = GlobalPlatformApi.AllocateMemory(InitialGapSize);
        Buffer->GapStart = 0;
        Buffer->GapEnd   = InitialGapSize;
        Buffer->End      = InitialGapSize;

        Buffer->Point = 0;
    }

    return Buffer;
}

internal void DeleteBuffer(buffer *Buffer) {
    Buffer->GapStart = 0;
    Buffer->GapEnd   = Buffer->End;
    Buffer->Point    = 0;
}

internal void FreeBuffer(buffer *Buffer) {
    GlobalPlatformApi.FreeMemory(Buffer->Data);
    GlobalPlatformApi.FreeMemory(Buffer);
}

internal u32 MoveBufferPosFoward(buffer *Buffer, u32 Pos) {
    Assert(Pos != Buffer->End);
    Pos++;
    if (Pos == Buffer->GapStart)
        Pos =  Buffer->GapEnd;
    
    return Pos;
}

internal void ShiftGapToCursor(buffer *Buffer, u32 Cursor) {
    u32 GapSize = GetBufferGapSize(Buffer);
    if (Cursor < Buffer->GapStart) {
        u32 GapDelta = Buffer->GapStart - Cursor;
        Buffer->GapStart -= GapDelta;
        Buffer->GapEnd   -= GapDelta;
        MoveMemory(Buffer->Data + Buffer->GapEnd, Buffer->Data + Buffer->GapStart, GapDelta);
    }
    else
    if (Cursor > Buffer->GapStart) {
        u32 GapDelta = Cursor - Buffer->GapStart;
        MoveMemory(Buffer->Data + Buffer->GapStart, Buffer->Data + Buffer->GapEnd, GapDelta);
        Buffer->GapStart += GapDelta;
        Buffer->GapEnd   += GapDelta;
    }
    Assert(GetBufferGapSize(Buffer) == GapSize);
    AssertBufferInvariants(Buffer);
}

internal void EnsureGapSize(buffer *Buffer, u32 Min) {
    if (GetBufferGapSize(Buffer) < Min) {
        ShiftGapToCursor(Buffer, GetBufferLen(Buffer));
        u32 NewEnd = Max(2 * Buffer->End, Buffer->End + Min - GetBufferGapSize(Buffer));
        void *Temp     = GlobalPlatformApi.AllocateMemory(NewEnd);
        CopyMemory(Temp, Buffer->Data, Buffer->End);
        GlobalPlatformApi.FreeMemory(Buffer->Data);
        Buffer->Data   = Temp;
        Buffer->GapEnd = NewEnd;
        Buffer->End    = NewEnd; 
    }
    Assert(GetBufferGapSize(Buffer) >= Min);
}

internal b32 ReplaceChar(buffer *Buffer, u32 Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor < GetBufferLen(Buffer)) {
        SetBufferChar(Buffer, Cursor, Char);
        return 1;
    }
    else {
        return 0;
    }
}

internal void InsertChar(buffer *Buffer, u32 Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    EnsureGapSize(Buffer, 1);
    ShiftGapToCursor(Buffer, Cursor);
    Buffer->Data[Buffer->GapStart] = Char;
    Buffer->GapStart++;
    if (Buffer->Point >= Cursor) Buffer->Point++;
}

internal b32 DeleteBackwardChar(buffer *Buffer, u32 Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor > 0) {
        ShiftGapToCursor(Buffer, Cursor);
        Buffer->GapStart--;
        if (Buffer->Point >= Cursor) Buffer->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal b32 DeleteFowardChar(buffer *Buffer, u32 Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor < GetBufferLen(Buffer)) {
        ShiftGapToCursor(Buffer, Cursor);
        Buffer->GapEnd++;
        if (Buffer->Point > Cursor) Buffer->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal finginline u32 GetNextCharCursor(buffer *Buffer, u32 Cursor) {
    if (Cursor < GetBufferLen(Buffer))
        return Cursor + 1;
    else
        return Cursor;
}

internal finginline u32 GetPrevCharCursor(buffer *Buffer, u32 Cursor) {
    if (Cursor > 0)
        return Cursor - 1;
    else
        return Cursor;
}

internal u32 GetBeginningOfLineCursor(buffer *Buffer, u32 Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    Cursor = GetPrevCharCursor(Buffer, Cursor);
    while (Cursor > 0) {
        c8  Char = GetBufferChar(Buffer, Cursor);
        if (Char == '\n') {
            return GetNextCharCursor(Buffer, Cursor);
        }
        Cursor = GetPrevCharCursor(Buffer, Cursor);
    }
    return 0;
}

internal u32 GetEndOfLineCursor(buffer *Buffer, u32 CurrentCursor) {
    AssertCursorInvariants(Buffer, CurrentCursor);
    while (CurrentCursor < GetBufferLen(Buffer)) {
        c8  Char = GetBufferChar(Buffer, CurrentCursor);
        if (Char == '\n') {
            return CurrentCursor;
        }
        CurrentCursor = GetNextCharCursor(Buffer, CurrentCursor);
    }
    return GetBufferLen(Buffer);
}

internal u32 GetBeginningOfNextLineCursor(buffer *Buffer, u32 CurrentCursor) {
    return GetNextCharCursor(Buffer, GetEndOfLineCursor(Buffer, CurrentCursor));
}

internal u32 GetEndOfPrevLineCursor(buffer *Buffer, u32 CurrentCursor) {
    return GetPrevCharCursor(Buffer, GetBeginningOfLineCursor(Buffer, CurrentCursor));
}

internal u32 GetBeginningOfPrevLineCursor(buffer *Buffer, u32 CurrentCursor) {
    return GetBeginningOfLineCursor(Buffer, GetPrevCharCursor(Buffer, GetBeginningOfLineCursor(Buffer, CurrentCursor)));
}

internal u32 GetCursorColumn(buffer *Buffer, u32 Cursor) {
    return Cursor - GetBeginningOfLineCursor(Buffer, Cursor);
}

internal u32 GetLineLen(buffer *Buffer, u32 Cursor) {
    return GetEndOfLineCursor(Buffer, Cursor) - GetBeginningOfLineCursor(Buffer, Cursor);
}

internal u32 GetBegginingOfBufferCursor(buffer *Buffer, u32 CurrentCursor) {
    return 0;
}

internal u32 GetEndOfBufferCursor(buffer *Buffer, u32 CurrentCursor) {
    return GetBufferLen(Buffer);
}

internal u32 GetBufferColumn(buffer *Buffer, u32 CurrentCursor) {
    return CurrentCursor - GetBeginningOfLineCursor(Buffer, CurrentCursor);
}

internal void DrawBuffer(buffer *Buffer, ui_ctx *Ctx, ui_style *Style) {
    renderer *Renderer = Ctx->Renderer;
    rv2 Pos = rv2_(Style->Padding.x, Renderer->TargetClipRect.y - 100);
    u32 BufferLen = GetBufferLen(Buffer);
    font *Font = &Renderer->Fonts[Style->Font];

    c8   Char;
    u32  Index;
    rv2  Offset;
    rect GlyphRect;

    r32 TempX = Pos.x;

    rect Caret = rect_(0, 0, 0, 0);

    for (u32 Cursor = 0; Cursor < BufferLen; Cursor++) {
        Char   = GetBufferChar(Buffer, Cursor);
        Index  = (Char - 32 >= 0)? Char - 32 : '?' - 32;
        Offset = Font->GlyphOffsets[Index];
        GlyphRect = rect_(Pos.x + Offset.x, Pos.y - Offset.y, GetVecComps(Font->GlyphRects[Index].Dim));
        
        if (Ctx->mLeftButtonIsDown) {
            if (IsInsideRect(Ctx->mPos, GlyphRect)) {
                Buffer->Point = Cursor;
                //todo:set current buffer
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

        DrawGlyph(Renderer, Style->Font, Index, GlyphRect.Pos, Style->DefaultTextColor);
    
        Pos.x += Font->GlyphAdvances[Index] + Style->CharSpacing;
    }

    DrawRect(Renderer, Caret, Style->DefaultTextColor);
}

internal void SaveBuffer(buffer *Buffer, c8 *Filename) {
    GlobalPlatformApi.WriteFile(Buffer->Data, Buffer->GapStart, Filename, 0);
    GlobalPlatformApi.WriteFile(Buffer->Data + Buffer->GapEnd, Buffer->End - Buffer->GapEnd, Filename, 1);
}

internal void LoadBuffer(buffer *Buffer, c8 *Filename) {
    if (Filename) {
        file File = GlobalPlatformApi.LoadFile(Filename);
        DeleteBuffer(Buffer);
        EnsureGapSize(Buffer, File.Size);
        //todo: unecessary CopyMemory?
        CopyMemory(Buffer->Data, File.Data, File.Size);
        Buffer->GapStart = File.Size;
        GlobalPlatformApi.FreeFile(File);
    }
}

///////////////////////////////////////////////////////////

typedef struct _editor_context {
    rv2 mPos;
    r32 dtFrame;
    u16 LastKeyComb;
    c8  LastChar;

    ui_style *uiStyle;
    ui_ctx   *uiCtx;

    buffer **Buffers;
    id       CurrentBuffer;
    u32 nCurrentBufferLine;
    u32 nCurrentBufferColumn;
    //todo: add buffer stuff, etc
} editor_context;

#define CMD_PROC(Name) void cmd_proc_##Name(editor_context *c)
typedef CMD_PROC(callback);

CMD_PROC(DoNothing) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    //note: does nothing for now, consider error message.
}

CMD_PROC(InsertChar) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point,
               c->LastChar);
}

CMD_PROC(DeleteCharFoward) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    DeleteFowardChar(Buffer, Buffer->Point);
}

CMD_PROC(DeleteCharBackward) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    DeleteBackwardChar(Buffer, Buffer->Point);
}

CMD_PROC(Indent) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point, '\t');
}

CMD_PROC(MoveCarretLeft) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetPrevCharCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretRight) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetNextCharCursor(Buffer, Buffer->Point);
}

global u32 GoalColumn = -1;

CMD_PROC(MoveCarretUp) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    if (GoalColumn == -1)
        GoalColumn = GetBufferColumn(Buffer, Buffer->Point);
    u32 BeginningOfPrevLine = GetBeginningOfPrevLineCursor(Buffer, Buffer->Point);
    u32 PrevLineLen         = GetLineLen(Buffer, BeginningOfPrevLine);
    Buffer->Point = BeginningOfPrevLine + Min(PrevLineLen, GoalColumn);
}

CMD_PROC(MoveCarretDown) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    if (GoalColumn == -1)
        GoalColumn = GetBufferColumn(Buffer, Buffer->Point);
    u32 BeginningOfNextLine = GetBeginningOfNextLineCursor(Buffer, Buffer->Point);
    u32 NextLineLen         = GetLineLen(Buffer, BeginningOfNextLine);
    Buffer->Point = Min(BeginningOfNextLine + Min(NextLineLen, GoalColumn), GetBufferLen(Buffer) - 1);
}

CMD_PROC(MoveCarretToLineStart) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point =
        GetBeginningOfLineCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToLineEnd) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetEndOfLineCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToBeginningOfBuffer) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetBegginingOfBufferCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToEndOfBuffer) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetEndOfBufferCursor(Buffer, Buffer->Point);
}

CMD_PROC(InsertNewLine) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point, '\n');
}

CMD_PROC(SaveFile) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    SaveBuffer(Buffer, "a.c");
    // DrawRect(ORIGIN_CENTERED, rv2_(100, 100), rv2_(50, 50), HexToColor(0xFA4080FF));
}

CMD_PROC(OpenFile) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    LoadBuffer(Buffer, "a.c");
    // DrawRect(ORIGIN_CENTERED, rv2_(100, 100), rv2_(50, 50), HexToColor(0x8040FAFF));
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
    KEY_CTRL  = 1 << 8,
    KEY_ALT   = 1 << 9,
    KEY_SHIFT = 1 << 10
} key;

#define KeyComb(BaseKey, Ctrl, Alt, Shift) (u16)(BaseKey) | ((u16)(Ctrl) << 8) | ((u16)(Alt)  << 9) | ((u16)(Shift) << 10)
#define Ctrl(Key)  KeyComb(Key, 1, 0, 0)
#define Alt(Key)   KeyComb(Key, 0, 1, 0)
#define Shift(Key) KeyComb(Key, 0, 0, 1)

internal void UpdateEditorContextInput(editor_context *c, platform *p) {
    c->uiCtx->mPos              = p->mPos;
    c->uiCtx->mLeftButtonIsDown = p->mLeft;
    c->mPos    = p->mPos;
    c->dtFrame = p->dtForFrame;
    
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
    if (p->kChar) {
        if (!p->kCtrl)
            Key = KEY_CHAR;
        else
            Key = KeyComb(p->Char, p->kCtrl, p->kAlt, p->kShift);
    }
    c->LastChar    = p->Char;
    c->LastKeyComb = Key;
}

global command Keymap[1024];

///////////////////////////////////////////////////////////

void DrawUi(editor_context *c) {
    uiBottomBar(c->uiCtx, c->uiStyle, "some_text.c", 0, 0, c->dtFrame);
    DrawBuffer(c->Buffers[0], c->uiCtx, c->uiStyle);
    // UiAddButton(c->uiCtx, c->uiStyle, c->mPos, "saaaaaaaaaaaaaaaaadg");
}

///////////////////////////////////////////////////////////s

typedef struct _app_state {
    renderer *Renderer;

    command *Keymap;
    editor_context Context;

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
    State->Context.uiCtx   = PlatformApi.AllocateMemory(sizeof(ui_ctx));
    State->Context.uiStyle = PlatformApi.AllocateMemory(sizeof(ui_style));
    State->Context.Buffers = PlatformApi.AllocateMemory(sizeof(buffer *)*2);
    State->Context.Buffers[0] = PlatformApi.AllocateMemory(sizeof(buffer));
    State->Context.Buffers[1] = PlatformApi.AllocateMemory(sizeof(buffer));
    
    State->Context.uiCtx->Renderer = State->Renderer;
    State->Context.uiCtx->Hot      = -1;
    State->Context.uiCtx->Clicked  = -1;
    State->Context.uiCtx->Last     = -1;
    State->Context.uiCtx->Current  = -1;

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

    Keymap[KEY_NONE] = command_(cmd_proc_DoNothing, "DoNothing");
    Keymap[KEY_CHAR] = command_(cmd_proc_InsertChar, "InsertChar");
    Keymap[KEY_DEL]  = command_(cmd_proc_DeleteCharFoward, "DeleteCharFoward");
    Keymap[KEY_BACK] = command_(cmd_proc_DeleteCharBackward, "DeleteCharBackward");
    Keymap[KEY_TAB]  = command_(cmd_proc_Indent, "Indent");
    Keymap[KEY_LEFT]  = command_(cmd_proc_MoveCarretLeft, "MoveCarretLeft");
    Keymap[KEY_RIGHT] = command_(cmd_proc_MoveCarretRight, "MoveCarretRight");
    Keymap[KEY_UP]    = command_(cmd_proc_MoveCarretUp, "MoreCarretUp");
    Keymap[KEY_DOWN]  = command_(cmd_proc_MoveCarretDown, "MoveCarretDown");
    Keymap[KEY_HOME]  = command_(cmd_proc_MoveCarretToLineStart, "MoveCarretToLineStart");
    Keymap[KEY_END]   = command_(cmd_proc_MoveCarretToLineEnd, "MoveCarretToLineEnd");
    Keymap[KEY_RETURN] = command_(cmd_proc_InsertNewLine, "InsertNewLine");
    Keymap[KEY_CTRL | 'S'] = command_(cmd_proc_SaveFile, "SaveFile");
    Keymap[KEY_CTRL | 'O'] = command_(cmd_proc_OpenFile, "OpenFile");    

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    i16 dMouseWheel = (State->dLastMouseWheel != p->dmWheel)? p->dmWheel : 0;
    State->dLastMouseWheel = p->dmWheel;
    State->Context.uiCtx->dmWheel = dMouseWheel;

    UpdateEditorContextInput(&State->Context, p);

    if (Keymap[State->Context.LastKeyComb].Proc)
        Keymap[State->Context.LastKeyComb].Proc(&State->Context);

    DrawUi(&State->Context);
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
