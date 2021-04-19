#ifndef BUFFER_H
#define BUFFER_H

#include "lingo.h"
#include "api.h"

#include "renderer.h"

typedef struct _buffer {
    u32 ReferenceCount;

    c8 *Data;
    u32 GapStart;
    u32 GapEnd;
    u32 End;
    
    c8 *Filename;
    u32 Point;
    b32 IsCurrent;
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
        Buffer->ReferenceCount = 1;

        Buffer->Data     = GlobalPlatformApi.AllocateMemory(InitialGapSize);
        Buffer->GapStart = 0;
        Buffer->GapEnd   = InitialGapSize;
        Buffer->End      = InitialGapSize;

        Buffer->Filename = Filename; Buffer->Point = 0;
    }

    return Buffer;
}

internal void DeleteBuffer(buffer *Buffer) {
    Buffer->GapStart = 0;
    Buffer->GapEnd   = Buffer->End;
    Buffer->Point    = 0;
}

internal void ReleaseBuffer(buffer *Buffer) {
    Assert(Buffer->ReferenceCount > 0);
    Buffer->ReferenceCount--;
    if (Buffer->ReferenceCount == 0) {
        GlobalPlatformApi.FreeMemory(Buffer->Data);
        GlobalPlatformApi.FreeMemory(Buffer);
    }
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

internal void DrawBuffer(renderer *Renderer, buffer *Buffer, id FontId, rv2 Pos,
                         r32 Size, r32 LineSpacing, r32 CharSpacing, colorb Color,
                         rv2 MousePos, b32 MouseLeftButton)
{
    u32 BufferLen = GetBufferLen(Buffer);

    font *Font = &Renderer->Fonts[FontId];
    rv2 Advance  = rv2_(0, 0);
    rv2 GlyphPos = rv2_(0, 0);

    rect Caret = rect_(0, 0, 0, 0);

    for (u32 Cursor = 0; Cursor < BufferLen; Cursor++) {
        u32 Index = 0;
        c8  Char  = GetBufferChar(Buffer, Cursor);

        Index = (Char - 32 > 0)? Char - 32 : ' ' - 32;

        GlyphPos.x = Pos.x +  Advance.x + Font->GlyphOffsets[Index].x;
        GlyphPos.y = Pos.y - (Advance.y + Font->GlyphOffsets[Index].y + Font->GlyphRects[Index].h);

        if (MouseLeftButton)
            if (IsInsideRect(MousePos, rect_(GlyphPos.x, GlyphPos.y + Font->GlyphRects[Index].h, Font->GlyphRects[Index].w, Font->GlyphRects[Index].h)))
                Buffer->Point = Cursor;

        if (Cursor == Buffer->Point && Buffer->IsCurrent)
            Caret = rect_(Pos.x +  Advance.x, Pos.y - Advance.y + Font->Descender, 2, Font->Ascender);

        if (Char == '\n') {
            if (Cursor == Buffer->Point && Buffer->IsCurrent)
                Caret = rect_(Pos.x +  Advance.x, Pos.y - Advance.y + Font->Descender, 2, Font->Ascender);
            Advance.y += Font->LineGap + LineSpacing;
            Advance.x  = 0;
            continue;
        }
        else {
            Advance.x += Font->GlyphAdvances[Index] + CharSpacing;
        }

        DrawGlyph(Renderer, FontId, Index, GlyphPos, Color);
    }

    DrawRect(Renderer, Caret, Color);
}

void OutputDebugBuffer(buffer *Buffer) {
    char temp[1024];
    CopyMemory(temp, Buffer->Data, Buffer->GapStart);
    temp[Buffer->GapStart] = 0;
    OutputDebugStringA(temp);
    CopyMemory(temp, Buffer->Data + Buffer->GapEnd, Buffer->End - Buffer->GapEnd);
    temp[Buffer->End - Buffer->GapEnd] = 0;
    OutputDebugStringA(temp);
}

internal void SaveBuffer(buffer *Buffer) {
    GlobalPlatformApi.WriteFile(Buffer->Data, Buffer->GapStart, Buffer->Filename, 0);
    GlobalPlatformApi.WriteFile(Buffer->Data + Buffer->GapEnd, Buffer->End - Buffer->GapEnd, Buffer->Filename, 1);
}

internal void LoadBuffer(buffer *Buffer) {
    Buffer->IsCurrent = 1;
    if (Buffer->Filename) {
        file File = GlobalPlatformApi.LoadFile(Buffer->Filename);
        DeleteBuffer(Buffer);
        EnsureGapSize(Buffer, File.Size);
        //todo: unecessary CopyMemory?
        CopyMemory(Buffer->Data, File.Data, File.Size);
        Buffer->GapStart = File.Size;
        GlobalPlatformApi.FreeFile(File);
    }
}

#endif//BUFFER_H

///////////////////////////////////////////////////////////

#ifndef COMMAND_H
#define COMMAND_H

typedef struct _command_context {
    /* DO NOT REMOVE */
        buffer *Buffers[2];
        c8      LastChar;
        id      CurrentBuffer;
        u16     NoBuffers;
        u32     GoalColumn;
        //todo: line number, column number, etc
    /* DO NOT REMOVE */
} command_context;

#define COMMAND_FUNC(Name) void CmdFunc_##Name(command_context *Ctx)

typedef void command_func(command_context *Ctx);

typedef struct _command {
    c8           *Desc;
    command_func *Func;
} command;

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

#define MAX_KEY_COMBS (1 << (8 + 3))
typedef struct _keymap {
    command Commands[MAX_KEY_COMBS];

} keymap;

internal command *GetKeyCommand(keymap *Keymap, u16 KeyComb) {
    Assert(KeyComb < MAX_KEY_COMBS);
    return Keymap->Commands + KeyComb;
}

internal finginline command NewCommand(command_func *Func, c8 *Description) {
    return (command){Description, Func};
}

#define Bind(k, KeyComb, CommandFunc, CommandDesc) \
    k->Commands[KeyComb] = NewCommand(CommandFunc, CommandDesc);

internal finginline u16 GetKeyComb(b8 Ctrl, b8 Alt, b8 Shift, key Key) {
    return (u16)Key | ((u16)Ctrl << 8) | ((u16)Alt << 9) | ((u16)Shift << 10);
}

internal finginline u16 Ctrl(u8 Key) {
    return GetKeyComb(1, 0, 0, Key);
}

internal finginline u16 Alt(u8 Key) {
    return GetKeyComb(0, 1, 0, Key);
}

internal finginline u16 Sh1ft(u8 Key) {
    return GetKeyComb(0, 0, 1, Key);
}

///////////////////////////////////////////////////////////

COMMAND_FUNC(DoNothing) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    //note: does nothing for now, consider error message.
}

COMMAND_FUNC(InsertChar) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point,
               Ctx->LastChar);
}

COMMAND_FUNC(DeleteCharFoward) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    DeleteFowardChar(Buffer, Buffer->Point);
}

COMMAND_FUNC(DeleteCharBackward) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    DeleteBackwardChar(Buffer, Buffer->Point);
}

COMMAND_FUNC(Indent) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point, '\t');
}

COMMAND_FUNC(MoveCarretLeft) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    Buffer->Point = GetPrevCharCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(MoveCarretRight) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    Buffer->Point = GetNextCharCursor(Buffer, Buffer->Point);
}

u32 GoalColumn = -1;

COMMAND_FUNC(MoveCarretUp) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    if (GoalColumn == -1)
        GoalColumn = GetBufferColumn(Buffer, Buffer->Point);
    u32 BeginningOfPrevLine = GetBeginningOfPrevLineCursor(Buffer, Buffer->Point);
    u32 PrevLineLen         = GetLineLen(Buffer, BeginningOfPrevLine);
    Buffer->Point = BeginningOfPrevLine + Min(PrevLineLen, GoalColumn);
}

COMMAND_FUNC(MoveCarretDown) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    if (GoalColumn == -1)
        GoalColumn = GetBufferColumn(Buffer, Buffer->Point);
    u32 BeginningOfNextLine = GetBeginningOfNextLineCursor(Buffer, Buffer->Point);
    u32 NextLineLen         = GetLineLen(Buffer, BeginningOfNextLine);
    Buffer->Point = BeginningOfNextLine + Min(NextLineLen, GoalColumn);
}

COMMAND_FUNC(MoveCarretToBeginningOfLine) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    Buffer->Point =
        GetBeginningOfLineCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(MoveCarretToEndOfLine) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    Buffer->Point = GetEndOfLineCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(MoveCarretToBeginningOfBuffer) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    Buffer->Point = GetBegginingOfBufferCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(MoveCarretToEndOfBuffer) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    Buffer->Point = GetEndOfBufferCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(InsertNewLine) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point, '\n');
}

COMMAND_FUNC(NextBuffer) {
    Ctx->Buffers[Ctx->CurrentBuffer]->IsCurrent = 0;
    if (Ctx->CurrentBuffer < Ctx->NoBuffers - 1)
        Ctx->CurrentBuffer++;
    Ctx->Buffers[Ctx->CurrentBuffer]->IsCurrent = 1;
}

COMMAND_FUNC(PrevBuffer) {
    Ctx->Buffers[Ctx->CurrentBuffer]->IsCurrent = 0;
    if (Ctx->CurrentBuffer > 0)
        Ctx->CurrentBuffer--;
    Ctx->Buffers[Ctx->CurrentBuffer]->IsCurrent = 1;
}

COMMAND_FUNC(SaveBuffer) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    SaveBuffer(Buffer);
    // DrawRect(ORIGIN_CENTERED, rv2_(100, 100), rv2_(50, 50), HexToColor(0xFA4080FF));
}

COMMAND_FUNC(LoadBuffer) {
    buffer *Buffer = Ctx->Buffers[Ctx->CurrentBuffer];
    LoadBuffer(Buffer);
    // DrawRect(ORIGIN_CENTERED, rv2_(100, 100), rv2_(50, 50), HexToColor(0x8040FAFF));
}

internal keymap *CreateKeymap() {
    keymap *Keymap           = GlobalPlatformApi.AllocateMemory(sizeof(keymap));
    command CommandDoNothing = {"do nothing", CmdFunc_DoNothing};
    for (u32 i = 0; i < MAX_KEY_COMBS; i++) {
        Keymap->Commands[i] = CommandDoNothing;
    }
    return Keymap;
}

internal keymap *CreateMyKeymap() {
    keymap *Keymap = CreateKeymap();

    for (c8 Key = 0; Key < 127; Key++) {
        if (IsPrintableChar(Key)) {
            Bind(Keymap, GetKeyComb(0, 0, 0, Key), CmdFunc_InsertChar, "insert char");
            Bind(Keymap, GetKeyComb(0, 0, 1, Key), CmdFunc_InsertChar, "insert char");
        }
    }

    Bind(Keymap, KEY_DEL,           CmdFunc_DeleteCharFoward,              "delete char foward");
    Bind(Keymap, KEY_BACK,          CmdFunc_DeleteCharBackward,            "delete char backward");
    Bind(Keymap, KEY_TAB,           CmdFunc_Indent,                        "indent");
    Bind(Keymap, KEY_LEFT,          CmdFunc_MoveCarretLeft,                "move carret left");
    Bind(Keymap, KEY_RIGHT,         CmdFunc_MoveCarretRight,               "move carret right");
    Bind(Keymap, KEY_UP,            CmdFunc_MoveCarretUp,                  "move carret left");
    Bind(Keymap, KEY_DOWN,          CmdFunc_MoveCarretDown,                "move carret right");
    Bind(Keymap, Ctrl(KEY_PG_UP),   CmdFunc_NextBuffer,                    "use next buffer");
    Bind(Keymap, Ctrl(KEY_PG_DOWN), CmdFunc_PrevBuffer,                    "use prev buffer");
    Bind(Keymap, KEY_HOME,          CmdFunc_MoveCarretToBeginningOfLine,   "move carret to begginning of line");
    Bind(Keymap, KEY_END,           CmdFunc_MoveCarretToEndOfLine,         "move carret to end of line");
    Bind(Keymap, Ctrl(KEY_HOME),    CmdFunc_MoveCarretToBeginningOfBuffer, "move carret to begginning of buffer");
    Bind(Keymap, Ctrl(KEY_END),     CmdFunc_MoveCarretToEndOfBuffer,       "move carret to end of buffer");
    Bind(Keymap, KEY_RETURN,        CmdFunc_InsertNewLine,                 "insert new line");
    Bind(Keymap, Ctrl('S'),         CmdFunc_SaveBuffer,                    "save buffer");
    Bind(Keymap, Ctrl('O'),         CmdFunc_LoadBuffer,                    "load buffer");
    
    return Keymap;
}

#endif//COMMAND_H
