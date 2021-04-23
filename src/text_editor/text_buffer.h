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
    u32 Point;

    b32 Active;
    b32 Loaded;
    id  Id;
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

        Buffer->Point = 0;
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

internal void DrawBuffer(renderer *Renderer, buffer *Buffer,
                         ui_ctx *Ctx, ui_style *Style,rv2 Pos) {
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

        if (Cursor == Buffer->Point && Buffer->Active)
            Caret = rect_(Pos.x, Pos.y + Font->Descender, 2, Font->Ascender);

        if (Char == ' ') {
            Pos.x += Font->GlyphAdvances[Index] + Style->CharSpacing;
            continue;
        }

        if (Char == '\r') {
            continue;
        }

        if (Char == '\n') {
            if (Cursor == Buffer->Point && Buffer->Active)
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

// void OutputDebugBuffer(buffer *Buffer) {
//     char temp[1024];
//     CopyMemory(temp, Buffer->Data, Buffer->GapStart);
//     temp[Buffer->GapStart] = 0;
//     OutputDebugStringA(temp);
//     CopyMemory(temp, Buffer->Data + Buffer->GapEnd, Buffer->End - Buffer->GapEnd);
//     temp[Buffer->End - Buffer->GapEnd] = 0;
//     OutputDebugStringA(temp);
// }

internal void SaveBuffer(buffer *Buffer, c8 *Filename) {
    GlobalPlatformApi.WriteFile(Buffer->Data, Buffer->GapStart, Filename, 0);
    GlobalPlatformApi.WriteFile(Buffer->Data + Buffer->GapEnd, Buffer->End - Buffer->GapEnd, Filename, 1);
}

internal void LoadBuffer(buffer *Buffer, c8 *Filename) {
    Buffer->Active = 1;
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

#endif//BUFFER_H
