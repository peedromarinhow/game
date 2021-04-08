#ifndef BUFFER_H
#define BUFFER_H

#include "lingo.h"
#include "api.h"
#include "graphics.h"

typedef struct _buffer {
    u32 ReferenceCount;

    c8 *Data;
    u32 GapStart;
    u32 GapEnd;
    u32 End;
    
    c8 *Filename;
    u32 Point;
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

internal buffer *CreateBuffer(u32 InitialGapSize) {
    buffer *Buffer = AllocateMemory(sizeof(buffer)); {
        Buffer->ReferenceCount = 1;

        Buffer->Data     = AllocateMemory(InitialGapSize);
        Buffer->GapStart = 0;
        Buffer->GapEnd   = InitialGapSize;
        Buffer->End      = InitialGapSize;

        Buffer->Filename = "a.c";
        Buffer->Point = 0;
    }

    return Buffer;
}

internal void ReleaseBuffer(buffer *Buffer) {
    Assert(Buffer->ReferenceCount > 0);
    Buffer->ReferenceCount--;
    if (Buffer->ReferenceCount == 0) {
        FreeMemory(Buffer->Data);
        FreeMemory(Buffer);
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
        u32 NewEnd = 2 * Buffer->End;
        void *Temp     = AllocateMemory(NewEnd);
        CopyMemory(Temp, Buffer->Data, Buffer->End);
        FreeMemory(Buffer->Data);
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
    if (Buffer->Point >= Cursor)
        Buffer->Point++;
}

internal b32 DeleteBackwardChar(buffer *Buffer, u32 Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor > 0) {
        ShiftGapToCursor(Buffer, Cursor);
        Buffer->GapStart--;
        if (Buffer->Point >= Cursor)
            Buffer->Point--;
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
        if (Buffer->Point > Cursor)
            Buffer->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal finginline u32 GetNextCursor(buffer *Buffer, u32 Cursor) {
    if (Cursor < GetBufferLen(Buffer)) {
        return Cursor + 1;
    }
    else {
        return Cursor;
    }
}

internal finginline u32 GetPrevCursor(buffer *Buffer, u32 Cursor) {
    if (Cursor > 0) {
        return Cursor - 1;
    }
    else {
        return Cursor;
    }
}

internal u32 GetBegginingOfLineCursor(buffer *Buffer, u32 Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    Cursor = GetPrevCursor(Buffer, Cursor);
    while (Cursor > 0) {
        c8  Char = GetBufferChar(Buffer, Cursor);
        if (Char == '\n') {
            return GetNextCursor(Buffer, Cursor);
        }
        Cursor = GetPrevCursor(Buffer, Cursor);
    }
    return 0;
}

internal u32 GetEndOfLineCursor(buffer *Buffer, u32 Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    while (Cursor < GetBufferLen(Buffer)) {
        c8  Char = GetBufferChar(Buffer, Cursor);
        if (Char == '\n') {
            return Cursor;
        }
        Cursor = GetNextCursor(Buffer, Cursor);
    }
    return GetBufferLen(Buffer);
}

internal void SaveBufferToFile(buffer *Buffer) {
    WriteFile_(Buffer->Data, Buffer->GapStart, Buffer->Filename, 0);
    WriteFile_(Buffer->Data + Buffer->GapEnd, Buffer->End - Buffer->GapEnd, Buffer->Filename, 1);
}

internal u32 CopyLineFromBuffer(c8 *Line, i32 MaxLineSize, buffer *Buffer, u32 *OutCursor) {
    u32 Cursor = *OutCursor;
    i32 i;
    for (i = 0; i < MaxLineSize && Cursor < GetBufferLen(Buffer); i++) {
        c8  Char = GetBufferChar(Buffer, Cursor);
        if (Char == '\n')
            break;

        Line[i] = Char;
        Cursor++;
    }
    while (Cursor < GetBufferLen(Buffer) && GetBufferChar(Buffer, Cursor) != '\n') {
        Cursor++;
    }
   *OutCursor = Cursor;
    return i;
}

internal void DrawBuffer(rv2 Pos, buffer *Buffer, font *Font, f32 LineHeight) {
    c8 Line[256];
    for (u32 Cursor = 0; Cursor < GetBufferLen(Buffer); Cursor++) {
        u32 LineLen = CopyLineFromBuffer(Line, sizeof(Line) - 1, Buffer, &Cursor);
        Line[LineLen] = '\0';
        DrawText_(Font, Line, Pos, Font->Size, 0, 0, HexToColor(0xFAFAFAFF));
        Pos.y -= LineHeight;
    }
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

#endif//BUFFER_H





#ifndef COMMAND_H
#define COMMAND_H

typedef struct _command_context {
    /* DO NOT REMOVE */ buffer *Buffer;
    //todo: line number, column number, etc
    c8 LastChar;
} command_context;

#define EDITOR_COMMAND_FUNC(Name) void Name(command_context Ctx)
typedef EDITOR_COMMAND_FUNC(command_func);

typedef struct _command {
    const c8     *Desc;
    command_func *Func;
} command;
#define MAX_KEY_COMBS (1 << (8 + 3))
typedef struct _keymap {
    command Commands[MAX_KEY_COMBS];
} keymap;

typedef enum _key {
    KEY_NONE = 0,
    KEY_DEL,
    KEY_BACK,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_RETURN,
    KEY_CHAR,
} key;

internal command *GetKeyCommand(keymap *Keymap, u16 KeyComb) {
    Assert(KeyComb < MAX_KEY_COMBS);
    return Keymap->Commands + KeyComb;
}

EDITOR_COMMAND_FUNC(CmdFunc_DoNothing) {
    //note: does nothing for now, consider error message.
}

EDITOR_COMMAND_FUNC(CmdFunc_InsertChar) {
    InsertChar(Ctx.Buffer, Ctx.Buffer->Point, Ctx.LastChar);
}

EDITOR_COMMAND_FUNC(CmdFunc_DeleteCharFoward) {
    DeleteFowardChar(Ctx.Buffer, Ctx.Buffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_DeleteCharBackward) {
    DeleteBackwardChar(Ctx.Buffer, Ctx.Buffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_MoveCarretLeft) {
    Ctx.Buffer->Point = GetPrevCursor(Ctx.Buffer, Ctx.Buffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_MoveCarretRight) {
    Ctx.Buffer->Point = GetNextCursor(Ctx.Buffer, Ctx.Buffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_MoveCarretToBegginningOfLine) {
    Ctx.Buffer->Point = GetBegginingOfLineCursor(Ctx.Buffer, Ctx.Buffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_MoveCarretToEndOfLine) {
    Ctx.Buffer->Point = GetEndOfLineCursor(Ctx.Buffer, Ctx.Buffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_InsertNewLine) {
    InsertChar(Ctx.Buffer, Ctx.Buffer->Point, '\n');
}

EDITOR_COMMAND_FUNC(CmdFunc_SaveBuffer) {
    SaveBufferToFile(Ctx.Buffer);
    DrawRect(ORIGIN_CENTERED, rv2_(100, 100), rv2_(50, 50), HexToColor(0xFA4080FF));
}

internal finginline command NewCommand(command_func *Func, c8 *Description) {
    return (command){Description, Func};
}

internal finginline u16 GetKeyComb(b8 Ctrl, b8 Alt, b8 Shift, key Key) {
    return (u16)Key | ((u16)Ctrl << 8) | ((u16)Alt << 9) | ((u16)Shift << 10);
}

#define BIND(k, KeyComb, CommandFunc, CommandDesc) \
    k->Commands[KeyComb] = NewCommand(CommandFunc, CommandDesc);

internal keymap *CreateKeymap() {
    keymap *Keymap           = AllocateMemory(sizeof(keymap));
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
            BIND(Keymap, GetKeyComb(0, 0, 0, Key), CmdFunc_InsertChar, "insert char");
            BIND(Keymap, GetKeyComb(0, 0, 1, Key), CmdFunc_InsertChar, "insert char");
        }
    }

    BIND(Keymap, KEY_DEL,    CmdFunc_DeleteCharFoward,             "delete char foward");
    BIND(Keymap, KEY_BACK,   CmdFunc_DeleteCharBackward,           "delete char backward");
    BIND(Keymap, KEY_LEFT,   CmdFunc_MoveCarretLeft,               "move carret left");
    BIND(Keymap, KEY_RIGHT,  CmdFunc_MoveCarretRight,              "move carret right");
    BIND(Keymap, KEY_HOME,   CmdFunc_MoveCarretToBegginningOfLine, "move carret to begginning of line");
    BIND(Keymap, KEY_END,    CmdFunc_MoveCarretToEndOfLine,        "move carret to end of line");
    BIND(Keymap, KEY_RETURN, CmdFunc_InsertNewLine,                "insert new line");
    BIND(Keymap, GetKeyComb(1, 0, 0, 'S'-64), CmdFunc_SaveBuffer, "save buffer");
                                    //hack
    
    return Keymap;
}

#endif//COMMAND_H