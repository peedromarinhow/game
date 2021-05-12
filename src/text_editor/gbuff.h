#ifndef GBUFF_h
#define GBUFF_h

#include "lingo.h"
#include "platform.h"
#include "colors.h"
#include "renderer.h"

typedef struct _cursor {
    u32 GapStart;
    u32 GapEnd;
    u32 Point;
} cursor;

typedef struct _gbuff {
    c8 *Data;
    u32 Size;

    u32 GapStart;
    u32 GapEnd;
    u32 Point;
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
    if (Cursor < gbuff_GetLen(Buff))
        return Cursor + gbuff_GetGapSize(Buff);
    else
        return gbuff_GetLen(Buff);
}

inline c8 gbuff_GetChar(gbuff *Buff, u32 Cursor) {
    return Buff->Data[gbuff_GetCursorIndex(Buff, Cursor)];
}

inline u32 gbuff_GetUtf32Char(gbuff *Buff, u32 Cursor) {
    return (Buff->Data[gbuff_GetCursorIndex(Buff, Cursor + 0)] >>  0) |
           (Buff->Data[gbuff_GetCursorIndex(Buff, Cursor + 1)] >>  8) |
           (Buff->Data[gbuff_GetCursorIndex(Buff, Cursor + 2)] >> 16) |
           (Buff->Data[gbuff_GetCursorIndex(Buff, Cursor + 3)] >> 24);
}

internal gbuff gbuff_Create(platform_api *Api, u32 InitialGapSize) {
    gbuff Result = {0};

    Result.Data     = Api->AllocateMemory(InitialGapSize + 1);
    Result.Size     = InitialGapSize;
    Result.GapStart = 0;
    Result.GapEnd   = InitialGapSize;
    Result.Point    = 0;

    Result.Data[InitialGapSize] = '\0';

    return Result;
}

internal void gbuff_Free(platform_api *Api, gbuff *Buff) {
    Api->FreeMemory(Buff->Data);
}

internal u32 gbuff_MovePosFoward(gbuff *Buff, u32 Pos) {
    if (Pos != Buff->Size)
        Pos++;
    if (Pos == Buff->GapStart)
        Pos =  Buff->GapEnd;
    return Pos;
}

internal void gbuff_ShiftGapToCursor(gbuff *Buff, u32 Cursor) {
    u32 GapSize = gbuff_GetGapSize(Buff);
    if (Cursor < Buff->GapStart) {
        u32 GapDelta = Buff->GapStart - Cursor;
        Buff->GapStart -= GapDelta;
        Buff->GapEnd   -= GapDelta;
        MoveMemory(Buff->Data + Buff->GapEnd, Buff->Data + Buff->GapStart, GapDelta);
    }
    else
    if (Cursor > Buff->GapStart) {
        u32 GapDelta = Cursor - Buff->GapStart;
        MoveMemory(Buff->Data + Buff->GapStart, Buff->Data + Buff->GapEnd, GapDelta);
        Buff->GapStart += GapDelta;
        Buff->GapEnd   += GapDelta;
    }
    Assert(gbuff_GetGapSize(Buff) == GapSize);
    // AssertBuffInvariants(Buff);
}

internal void gbuff_EnsureGapSize(platform_api *Api, gbuff *Buff, u32 Min) {
    if (gbuff_GetGapSize(Buff) < Min) {
        gbuff_ShiftGapToCursor(Buff, gbuff_GetLen(Buff));
        u32 NewEnd = Max(2 * Buff->Size, Buff->Size + Min - gbuff_GetGapSize(Buff));
        void *Temp = Api->AllocateMemory(NewEnd);
        CopyMemory(Temp, Buff->Data, Buff->Size);
        Api->FreeMemory(Buff->Data);
        Buff->Data   = Temp;
        Buff->GapEnd = NewEnd;
        Buff->Size   = NewEnd; 
    }
    Assert(gbuff_GetGapSize(Buff) >= Min);
}

internal b32 gbuff_ReplaceChar(gbuff *Buff, u32 Cursor, c8 Char) {
    // AssertCursorInvariants(Buff, Cursor);
    if (Cursor < gbuff_GetLen(Buff)) {
        // SetBuffChar(Buff, Cursor, Char);
        return 1;
    }
    else {
        return 0;
    }
}

internal void gbuff_InsertChar(platform_api *Api, gbuff *Buff, u32 Cursor, c8 Char) {
    // AssertCursorInvariants(Buff, Cursor);
    gbuff_EnsureGapSize(Api, Buff, 1);
    gbuff_ShiftGapToCursor(Buff, Cursor);
    Buff->Data[Buff->GapStart] = Char;
    Buff->GapStart++;
    if (Buff->Point >= Cursor) Buff->Point++;
}

internal b32 gbuff_DeleteBackwardChar(gbuff *Buff, u32 Cursor) {
    // AssertCursorInvariants(Buff, Cursor);
    if (Cursor > 0) {
        gbuff_ShiftGapToCursor(Buff, Cursor);
        Buff->GapStart--;
        if (Buff->Point >= Cursor) Buff->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal b32 gbuff_DeleteFowardChar(gbuff *Buff, u32 Cursor) {
    // AssertCursorInvariants(Buff, Cursor);
    if (Cursor < gbuff_GetLen(Buff)) {
        gbuff_ShiftGapToCursor(Buff, Cursor);
        Buff->GapEnd++;
        if (Buff->Point > Cursor) Buff->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal void gbuff_Save(platform_api *Api, gbuff *Buff, c8 *Filename) {
    Api->WriteFile(Buff->Data, Buff->GapStart, Filename, 0);
    Api->WriteFile(Buff->Data + Buff->GapEnd, Buff->Size - Buff->GapEnd, Filename, 1);
}

internal void gbuff_Load(platform_api *Api, gbuff *Buff, c8 *Filename) {
    if (Filename) {
        file File = Api->LoadFile(Filename);
        gbuff_Free(Api, Buff);
        gbuff_EnsureGapSize(Api, Buff, File.Size);
        //todo: unecessary CopyMemory?
        CopyMemory(Buff->Data, File.Data, File.Size);
        Buff->GapStart = File.Size;
        Api->FreeFile(File);
    }
}

inline u32 gbuff_GetNextCharCursor(gbuff *Buff, u32 Cursor) {
    if (Cursor < gbuff_GetLen(Buff)) {
        Cursor++;
        if (gbuff_GetChar(Buff, Cursor) == '\r')
            Cursor++;
    }
    return Cursor;
}

inline u32 gbuff_GetPrevCharCursor(gbuff *Buff, u32 Cursor) {
    if (Cursor > 0) {
        Cursor--;
        if (gbuff_GetChar(Buff, Cursor) == '\r')
            Cursor--;
    }
    return Cursor;
}

inline u32 gbuff_GetBeginningOfLineCursor(gbuff *Buff, u32 Cursor) {
    // AssertCursorInvariants(Buff, Cursor);
    Cursor = gbuff_GetPrevCharCursor(Buff, Cursor);
    while (Cursor > 0) {
        c8  Char = gbuff_GetChar(Buff, Cursor);
        if (Char == '\n') {
            return gbuff_GetNextCharCursor(Buff, Cursor);
        }
        Cursor = gbuff_GetPrevCharCursor(Buff, Cursor);
    }
    return 0;
}

inline u32 gbuff_GetEndOfLineCursor(gbuff *Buff, u32 Cursor) {
    // AssertCursorInvariants(Buff, Cursor);
    while (Cursor < gbuff_GetLen(Buff)) {
        c8  Char = gbuff_GetChar(Buff, Cursor);
        if (Char == '\n') {
            return Cursor;
        }
        Cursor = gbuff_GetNextCharCursor(Buff, Cursor);
    }
    return gbuff_GetLen(Buff);
}

inline u32 gbuff_GetBeginningOfNextLineCursor(gbuff *Buff, u32 Cursor) {
    return gbuff_GetNextCharCursor(Buff, gbuff_GetEndOfLineCursor(Buff, Cursor));
}

inline u32 gbuff_GetEndOfPrevLineCursor(gbuff *Buff, u32 Cursor) {
    return gbuff_GetPrevCharCursor(Buff, gbuff_GetBeginningOfLineCursor(Buff, Cursor));
}

inline u32 gbuff_GetBeginningOfPrevLineCursor(gbuff *Buff, u32 Cursor) {
    return gbuff_GetBeginningOfLineCursor(Buff, gbuff_GetPrevCharCursor(Buff, gbuff_GetBeginningOfLineCursor(Buff, Cursor)));
}

inline u32 gbuff_GetCursorColumn(gbuff *Buff, u32 Cursor) {
    return Cursor - gbuff_GetBeginningOfLineCursor(Buff, Cursor);
}

inline u32 gbuff_GetLineLen(gbuff *Buff, u32 Cursor) {
    return gbuff_GetEndOfLineCursor(Buff, Cursor) - gbuff_GetBeginningOfLineCursor(Buff, Cursor);
}

inline u32 gbuff_GetBegginingCursor(gbuff *Buff, u32 Cursor) {
    return 0;
}

inline u32 gbuff_GetEndCursor(gbuff *Buff, u32 Cursor) {
    return gbuff_GetLen(Buff);
}

inline u32 gbuff_GetColumn(gbuff *Buff, u32 Cursor) {
    return Cursor - gbuff_GetBeginningOfLineCursor(Buff, Cursor);
}

internal void gbuff_Render(renderer *Renderer, platform_api *Api, gbuff *Buff, rv2 Pos, rect *Caret) {
    rv2   Result = {-100000, -100000};
    font *Font = &Renderer->Fonts[0];
    r32   ScaleFactor = Font->Height/Font->Height;

    for (u32 Cursor = 0; Cursor <= gbuff_GetLen(Buff); Cursor++) {
        c8 Utf8Quartet[4];
        Utf8Quartet[0] = gbuff_GetChar(Buff, Cursor + 0);
        Utf8Quartet[1] = gbuff_GetChar(Buff, Cursor + 1);
        Utf8Quartet[2] = gbuff_GetChar(Buff, Cursor + 2);
        Utf8Quartet[3] = gbuff_GetChar(Buff, Cursor + 3);
        u32 NoCodepointBytes = 0;
        u32 Codepoint = GetNextCodepoint(Utf8Quartet, &NoCodepointBytes);
        u32 Index = GetGlyphIndex(Font, Codepoint);
        if (Codepoint == 0x3f) NoCodepointBytes = 1;
    
        r32 Advance = Font->Advances[Index];
        rv2 Bearing = rv2_(Font->Bearings[Index].x * ScaleFactor,
                           Font->Bearings[Index].y * ScaleFactor);
        rect Rect = rect_(Font->Rects[Index].x * ScaleFactor, Font->Rects[Index].y * ScaleFactor,
                          Font->Rects[Index].w * ScaleFactor, Font->Rects[Index].h * ScaleFactor);

        rv2 GlyphPos = rv2_(Pos.x + Bearing.x, Pos.y - (Rect.h - Bearing.y) - ((Font->Ascender>>6) + (Font->Descender>>6))/4);
        
        if (Cursor == Buff->Point)
           *Caret = rect_(Pos.x, Pos.y - Font->Height/2, 2, Font->Height);

        if (Utf8Quartet[0] == '\n') {
            Pos = rv2_(16, Pos.y - Font->Height);
            continue;
        }
    
        if (Cursor == gbuff_GetLen(Buff))
            break;
        DrawGlyph(Renderer, 0, Index, GlyphPos, GREY_50/*, ScaleFactor*/);

        Pos.x  += (i32)Advance * ScaleFactor;
        Cursor += (NoCodepointBytes - 1);
    }
}

#endif//GBUFF_h