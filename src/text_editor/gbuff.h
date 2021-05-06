#ifndef GBUFF_h
#define GBUFF_h

#include "lingo.h"
#include "platform.h"

typedef struct _cursor {
    u32 GapStart;
    u32 GapEnd;
    u32 Point;
} cursor;

typedef struct _gbuff {
    c8 *Data;
    u32 Size;

    c8 *GapStart;
    c8 *GapEnd;
    c8 *Point;
} gbuff;

inline u32 gbuff_GetGapSize(gbuff *Buff) {
    return Buff->GapEnd - Buff->GapStart;
}

inline u32 gbuff_GetLen(gbuff *Buff) {
    return Buff->Size - gbuff_GetGapSize(Buff);
}

inline u32 gbuff_GetCursorIndex(gbuff *Buff, u32 Cursor) {
    if (Cursor < Buff->GapStart)
        return Cursor;
    else
        return Cursor + gbuff_GetGapSize(Buff);
}

internal gbuff gbuff_Create(platform_api *Api, u32 InitialGapSize) {
    gbuff Result = {0};

    Result.Data     = Api->AllocateMemory(InitialGapSize);
    Result.Size     = InitialGapSize;
    Result.GapStart = 0;
    Result.GapEnd   = InitialGapSize;
    Result.Point    = 0;

    return Result;
}

internal void gbuff_Free(platform_api *Api, gbuff *Buffer) {
    Api->FreeMemory(Buffer->Data);
}

internal u32 gbuff_MovePosFoward(gbuff *Buffer, u32 Pos) {
    if (Pos != Buffer->End)
        Pos++;
    if (Pos == Buffer->GapStart)
        Pos =  Buffer->GapEnd;
    return Pos;
}

internal void gbuff_ShiftGapToCursor(gbuff *Buffer, u32 Cursor) {
    u32 GapSize = gbuff_GetGapSize(Buffer);
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
    Assert(gbuff_GetGapSize(Buffer) == GapSize);
    AssertBufferInvariants(Buffer);
}

internal void gbuff_EnsureGapSize(platform_api *Api, gbuff *Buffer, u32 Min) {
    if (gbuff_GetGapSize(Buffer) < Min) {
        gbuff_ShiftGapToCursor(Buffer, GetBufferLen(Buffer));
        u32 NewEnd = Max(2 * Buffer->End, Buffer->End + Min - gbuff_GetGapSize(Buffer));
        void *Temp     = GlobalPlatformApi.AllocateMemory(NewEnd);
        CopyMemory(Temp, Buffer->Data, Buffer->End);
        GlobalPlatformApi.FreeMemory(Buffer->Data);
        Buffer->Data   = Temp;
        Buffer->GapEnd = NewEnd;
        Buffer->End    = NewEnd; 
    }
    Assert(gbuff_GetGapSize(Buffer) >= Min);
}

internal b32 gbuff_ReplaceChar(gbuff *Buffer, u32 Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor < GetBufferLen(Buffer)) {
        SetBufferChar(Buffer, Cursor, Char);
        return 1;
    }
    else {
        return 0;
    }
}

internal void gbuff_InsertChar(platform_api *Api, gbuff *Buffer, u32 Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    gbuff_EnsureGapSize(Api, Buffer, 1);
    gbuff_ShiftGapToCursor(Buffer, Cursor);
    Buffer->Data[Buffer->GapStart] = Char;
    Buffer->GapStart++;
    if (Buffer->Point >= Cursor) Buffer->Point++;
}

internal b32 gbuff_DeleteBackwardChar(gbuff *Buffer, u32 Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor > 0) {
        gbuff_ShiftGapToCursor(Buffer, Cursor);
        Buffer->GapStart--;
        if (Buffer->Point >= Cursor) Buffer->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal b32 gbuff_DeleteFowardChar(gbuff *Buffer, u32 Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor < GetBufferLen(Buffer)) {
        gbuff_ShiftGapToCursor(Buffer, Cursor);
        Buffer->GapEnd++;
        if (Buffer->Point > Cursor) Buffer->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal void gbuff_Save(gbuff *Buffer, c8 *Filename) {
    GlobalPlatformApi.WriteFile(Buffer->Data, Buffer->GapStart, Filename, 0);
    GlobalPlatformApi.WriteFile(Buffer->Data + Buffer->GapEnd, Buffer->End - Buffer->GapEnd, Filename, 1);
}

internal void gbuff_Load(platform_api *Api, gbuff *Buffer, c8 *Filename) {
    if (Filename) {
        file File = Api->LoadFile(Filename);
        DeleteBuffer(Buffer);
        gbuff_EnsureGapSize(Api, Buffer, File.Size);
        //todo: unecessary CopyMemory?
        CopyMemory(Buffer->Data, File.Data, File.Size);
        Buffer->GapStart = File.Size;
        GlobalPlatformApi.FreeFile(File);
    }
}

#endif//GBUFF_h