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

//something *very* wrong here
internal void ShiftGapToPosition(buffer *Buffer, u32 Pos) {
    u32 Delta = 0;
    if (Pos < Buffer->Cursor.GapStart) {
        Delta = Buffer->Cursor.GapStart - Pos;
        Buffer->Cursor.GapStart -= Delta;
        Buffer->Cursor.GapEnd   -= Delta;
        MoveMemory(Buffer->Data + Buffer->Cursor.GapEnd, Buffer->Data + Buffer->Cursor.GapStart, Delta);
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
    Buffer->Cursor.GapStart++;
    Buffer->Cursor.Pos++;
}

void RemoveChar(buffer *Buffer) {
    if (Buffer->Cursor.Pos > 0) {
        ShiftGapToPosition(Buffer, Buffer->Cursor.Pos);
        Buffer->Data[Buffer->Cursor.GapStart] = '@';
        Buffer->Cursor.GapStart--;
        Buffer->Cursor.Pos--;
    }
}

void DebugDrawBuffer(buffer *Buffer, font *Font) {
    i32  ScaleFactor = 1;//Size/Font->Size;
    f32 CharOffset  = 0;
    f32 LineOffset  = 0;

    color4f Tint = WHITE4F;
    rv2     Pos  = Rv2(16, 32);

    glBindTexture(GL_TEXTURE_2D, Font->Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    u32 i = 0;
    while (i <= Buffer->End) {
        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
        i32 Index     = GetGlyphIndex(*Font, Buffer->Data[i]);
        i32 Codepoint = Font->Chars[Index].Codepoint;
        i32 OffY      = Font->Chars[Index].OffY;
        i32 OffX      = Font->Chars[Index].OffX;

        if (Buffer->Data[i] == '\t') {
            CharOffset += Font->Size * 1.5f;
            i++;
            continue; //note: skips the rest, so the caracter is not drawn
        }

        if (Buffer->Data[i] == '\n') {
            LineOffset += Font->Size;// + (LineSpacing * ScaleFactor);
            CharOffset = 0;
            i++;

            continue; //note: skips the rest, so the caracter is not drawn
        }

        if (Buffer->Data[i] == '\r') {
            i++;
            continue;
        }

        rectf32 Rect = Font->Rects[Index];
        f32 w        = Font->Texture.w;
        f32 h        = Font->Texture.h;

        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS); {
            glTexCoord2f(Rect.x/w, Rect.y/h);
            glVertex2f  (Pos.x + CharOffset + OffX,
                         Pos.y + LineOffset + OffY);

            glTexCoord2f((Rect.x + Rect.w)/w, Rect.y/h);
            glVertex2f  (Pos.x + CharOffset + Rect.w + OffX,
                         Pos.y + LineOffset + OffY);

            glTexCoord2f((Rect.x + Rect.w)/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharOffset + Rect.w + OffX,
                         Pos.y + LineOffset + Rect.h + OffY);

            glTexCoord2f(Rect.x/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharOffset + OffX,
                         Pos.y + LineOffset + Rect.h + OffY);
        } glEnd();
        glDisable(GL_TEXTURE_2D);

        glLineWidth(2);
        if (i == Buffer->Cursor.Pos - 1) {
            glBegin(GL_LINES); {
                glColor4f(Tint.r, Tint.g/2, Tint.b/2, Tint.a);
                glVertex2f(Pos.x + CharOffset + Rect.w + OffX, Pos.y + LineOffset + OffY);
                glVertex2f(Pos.x + CharOffset + Rect.w + OffX, Pos.y + LineOffset + Rect.h + OffY);
                glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
            } glEnd();
        }

        if (i == Buffer->Cursor.GapStart - 1) {
            glBegin(GL_LINES); {
                glColor4f(Tint.r/2, Tint.g/2, Tint.b, Tint.a);
                glVertex2f(Pos.x + CharOffset + Rect.w + OffX, Pos.y + LineOffset + OffY);
                glVertex2f(Pos.x + CharOffset + Rect.w + OffX, Pos.y + LineOffset + Rect.h + OffY);
                glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
            } glEnd();
        }

        if (i > Buffer->Cursor.GapStart && i < Buffer->Cursor.GapEnd) {
            glColor4f(Tint.r/2, Tint.g/2, Tint.b/2, Tint.a);
        }

        CharOffset += (Font->Chars[Index].Advance * ScaleFactor);// + (CharSpacing * ScaleFactor);

        // if (i >  Buffer->Cursor.GapStart)
        //     i += Buffer->Cursor.GapEnd - Buffer->Cursor.GapStart;
        // else
            i++;
    }
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