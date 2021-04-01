#ifndef BUFFER_H
#define BUFFER_H

#include "lingo.h"
#include "app.h"
#include "graphics.h"
#include "fonts.h"

typedef struct _cursor {
    u32 Pos;
    u32 GapStart;
    u32 GapEnd;
} cursor;
 
typedef struct _buffer {
    c8    *Data;
    cursor Cursor;
    u32    End;
} buffer;

internal buffer CreateBuffer(u32 Size) {
    buffer Result = {0}; {
        Result.Data            = AllocateMemory(Size);
        Result.Cursor.Pos      = 0;
        Result.Cursor.GapStart = 0;
        Result.Cursor.GapEnd   = Size;
        Result.End             = Size;
    }

    return Result;
}

internal void ShiftGapToPosition(buffer *Buffer, u32 Pos) {
    u32 Delta = 0;
    if (Pos < Buffer->Cursor.GapStart) {
        Delta = Buffer->Cursor.GapStart - Pos;
        MoveMemory(Buffer->Data + Buffer->Cursor.GapStart, Buffer->Data + Pos, Delta);
        Buffer->Cursor.GapStart -= Delta;
        Buffer->Cursor.GapEnd   -= Delta;
    }
    if (Pos > Buffer->Cursor.GapStart) {
        Delta = Pos - Buffer->Cursor.GapStart;
        MoveMemory(Buffer->Data + Buffer->Cursor.Pos, Buffer->Data + Buffer->Cursor.GapEnd, Delta);
        Buffer->Cursor.GapStart += Delta;
        Buffer->Cursor.GapEnd   += Delta;
    }
}

internal void EnsureGapSize(buffer *Buffer, u32 MinimumGapSize) {
    u32 GapSize = Buffer->Cursor.GapEnd - Buffer->Cursor.GapStart;
    if (GapSize < MinimumGapSize) {
        ShiftGapToPosition(Buffer, Buffer->End - GapSize);
        FreeMemory(Buffer->Data);
        u32 NewEndPosition = 2 * Buffer->End;
        Buffer->Data          = AllocateMemory(NewEndPosition);
        Buffer->Cursor.GapEnd = NewEndPosition;
        Buffer->End           = NewEndPosition;
    }
}

void InsertChar(buffer *Buffer, c8 Char) {
    EnsureGapSize(Buffer, 8);
    ShiftGapToPosition(Buffer, Buffer->Cursor.Pos);
    Buffer->Data[Buffer->Cursor.GapStart] = Char;
    Buffer->Cursor.Pos++;
}

void DebugDrawBuffer(buffer *Buffer, font *Font) {
    gDrawText(Font, Buffer->Data, Rv2(16, 32), Font->Size, 0, 0, WHITE4F, NULL);
    gDrawText(Font, Buffer->Data + Buffer->Cursor.GapEnd, Rv2(16, 64), Font->Size, 0, 0, WHITE4F, NULL);
}

#if 0
typedef u32 pos;
typedef struct _buffer {
    c8 *Data;
    pos End;
    pos GapStart;
    pos GapEnd;
} buffer;

internal void InitializeBuffer(buffer *Buffer, u32 InitialGapSize) {
    Buffer->Data             = AllocateMemory(InitialGapSize);
    Buffer->GapStart = 0;
    Buffer->GapEnd   = InitialGapSize;
    Buffer->End      = InitialGapSize;
}

internal void ShiftGapToPosition(buffer *Buffer, pos Position) {
    if (Position < Buffer->GapStart) {
        u32 GapDelta = Buffer->GapStart - Position;
        Buffer->GapStart -= GapDelta;
        Buffer->GapEnd   -= GapDelta;
        MoveMemory(Buffer->Data + Buffer->GapEnd, Buffer->Data + Buffer->GapStart, GapDelta);
    }
    else
    if (Position > Buffer->GapStart) {
        u32 GapDelta = Position - Buffer->GapStart;
        MoveMemory(Buffer->Data + Buffer->GapStart, Buffer->Data + Buffer->GapEnd, GapDelta);
        Buffer->GapStart += GapDelta;
        Buffer->GapEnd   += GapDelta;
    }
}

internal void EnsureGapSize(buffer *Buffer, u32 MinimumGapSize) {
    u32 GapSize = Buffer->GapEnd - Buffer->GapStart;
    if (GapSize < MinimumGapSize) {
        ShiftGapToPosition(Buffer, Buffer->End - GapSize);
        FreeMemory(Buffer->Data);
        u32 NewEndPosition = 2 * Buffer->End;
        Buffer->Data   = AllocateMemory(NewEndPosition);
        Buffer->GapEnd = NewEndPosition;
        Buffer->End    = NewEndPosition;
    }
}

internal void InsertString(buffer *Buffer, pos Cursor, c8 *String) {
    while (*String) {
        InsertCharacter(Buffer, Cursor, *String);
        String++;
    }
}
#endif

#endif//BUFFER_H