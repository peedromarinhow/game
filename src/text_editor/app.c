#include "lingo.h"
#include "app.h"

#include "maths.h"
#include "platform.h"
#include "memory.h"
#include "opengl.h"

#include "graphics.h"
#include "fonts.h"

internal void SwapMemory(c8 *Src, c8 *Dest, u32 Size) {
    c8 *Tmp = AllocateMemory(Size);
    for (u32 i = 0; i < Size; i++) {
        Tmp[i]  = Src[i];
        Src[i]  = Dest[i];
        Dest[i] = Tmp[i]; 
    }
    FreeMemory((void *)Tmp);
}

typedef struct _cursor {
    u32 Pos;
    u32 GapStart;
    u32 GapEnd;
} cursor;
 
typedef struct _buffer {
    c8 *Data;
    u32 GapStart;
    u32 GapEnd;
    u32 End;
} buffer;

internal buffer CreateBuffer(u32 Size) {
    buffer Result = {0}; {
        Result.Data      = AllocateMemory(Size);
        Result.GapStart  = 0;
        Result.GapEnd    = Size;
        Result.End       = Size;
    }

    return Result;
}

internal void BufferShiftGapToPos(buffer *Buffer, u32 Pos) {
    Buffer->GapStart = Pos;
}

void InsertChar(buffer *Buffer, u32 Pos, c8 Char) {
    if (1 /* todo: figure out how to not overflow */) {
        Buffer->Data[Buffer->GapStart] = Char;
        BufferShiftGapToPos(Buffer, Buffer->GapStart + 1);
    }
}

void RemoveChar(buffer *Buffer) {
    if (Buffer->Data && Buffer->GapStart > 0) {
        Buffer->Data[Buffer->GapStart] = 0;
        Buffer->GapStart--;
    }
}

void gDrawBuffer(buffer *Buffer, font *Font, rv2 Pos, r32 Size, r32 LineSpacing, r32 CharSpacing, color4f Tint) {
    f32 ScaleFactor = Size/Font->Size;
    f32 CharOffset  = 0;
    f32 LineOffset  = 0;

    glBindTexture(GL_TEXTURE_2D, Font->Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
        for (u32 i = 0; i < Buffer->GapStart; i++) {
            i32 Index     = GetGlyphIndex(*Font, Buffer->Data[i]);
            i32 Codepoint = Font->Chars[Index].Codepoint;
            i32 OffY      = Font->Chars[Index].OffY;
            i32 OffX      = Font->Chars[Index].OffX;

            if (Buffer->Data[i] == '\t') {
                CharOffset += (Font->Size + Font->Size/2);
                continue; //note: skips the rest, so the caracter is not drawn
            }

            if (Buffer->Data[i] == '\n') {
                LineOffset += Font->Size + (LineSpacing * ScaleFactor);
                CharOffset = 0;
                continue; //note: skips the rest, so the caracter is not drawn
            }

            if (Buffer->Data[i] == '\r') {
                i++;
                continue;
            }

            rectf32 Rect = Font->Rects[Index];
            f32 w        = Font->Texture.w;
            f32 h        = Font->Texture.h;

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

            CharOffset += (Font->Chars[Index].Advance * ScaleFactor) +
                          (CharSpacing * ScaleFactor);
        }
    } glEnd();
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        glVertex2f(2 + Pos.x + CharOffset, Pos.y + LineOffset - (0.75f * Font->Size));
        glVertex2f(2 + Pos.x + CharOffset + 2, Pos.y + LineOffset - (0.75f * Font->Size));
        glVertex2f(2 + Pos.x + CharOffset + 2, Pos.y + LineOffset + (0.15f * Font->Size));
        glVertex2f(2 + Pos.x + CharOffset, Pos.y + LineOffset + (0.15f * Font->Size));
    } glEnd();
    //todo: use gDrawLine
}

typedef struct _app_state {
    font   RobotoMono;
    file   TestFile;
    rv2    TestFilePos;
    // buffer Buffer;
} app_state;

external APP_INIT(Init) {
    Assert(sizeof(app_state) <= p->Memory.Size);
    app_state *State = (app_state *)p->Memory.Contents;

    AllocateMemory    = p->AllocateMemoryCallback;
    FreeMemory        = p->FreeMemoryCallback;
    LoadFile          = p->LoadFileCallback;
    FreeFile          = p->FreeFileCallback;
    LoadFileToArena   = p->LoadFileToArenaCallback;
    FreeFileFromArena = p->FreeFileFromArenaCallback;
    WriteFile_        = p->WriteFileCallback;
    ReportError       = p->ReportErrorCallback;
    ReportErrorAndDie = p->ReportErrorAndDieCallback;

    State->RobotoMono  = LoadFont("roboto_mono.ttf", 400, 32);
    State->TestFile    = LoadFile(__FILE__);
    State->TestFilePos = Rv2(16, 32);
    // State->Buffer     = CreateBuffer(16);

    // InsertChar(&State->Buffer, 1, 'L');
    // InsertChar(&State->Buffer, 0, 'I');
    // InsertChar(&State->Buffer, 3, 'B');
    // InsertChar(&State->Buffer, 2, 'E');
    // InsertChar(&State->Buffer, 5, 'R');
    // InsertChar(&State->Buffer, 4, 'A');
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;

    AllocateMemory    = p->AllocateMemoryCallback;
    FreeMemory        = p->FreeMemoryCallback;
    LoadFile          = p->LoadFileCallback;
    FreeFile          = p->FreeFileCallback;
    LoadFileToArena   = p->LoadFileToArenaCallback;
    FreeFileFromArena = p->FreeFileFromArenaCallback;
    WriteFile_        = p->WriteFileCallback;
    ReportError       = p->ReportErrorCallback;
    ReportErrorAndDie = p->ReportErrorAndDieCallback;
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;
    gBegin(Rv2(0, 0), p->WindowDimensions, Color4f(0.1f, 0.2f, 0.25f, 1));
    
    // if (p->KeyboardCharacterCame) {
    //     if (p->KeyboardCharacter == '\b')
    //         RemoveChar(&State->Buffer);
    //     else
    //         InsertChar(&State->Buffer, State->Buffer.GapStart, p->KeyboardCharacter);
    // }

    if (p->dMouseWheel > 0) {
        State->TestFilePos.y += State->RobotoMono.Size/2;
    }
    else
    if (p->dMouseWheel < 0) {
        State->TestFilePos.y -= State->RobotoMono.Size/2;
    }
    else
        State->TestFilePos.y += 0;

    gDrawText(&State->RobotoMono, State->TestFile.Data, State->TestFilePos,
               State->RobotoMono.Size, 0, 0, Color4f(1, 1, 1, 1), NULL);

    // gDrawBuffer(&State->Buffer, &State->RobotoMono, Rv2(State->RobotoMono.Size/2, State->RobotoMono.Size), State->RobotoMono.Size, 0, 0, Color4f(1, 1, 1, 1));
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}

#if 0
typedef struct _buffer {
    c8 *Data;
    u32 GapStart;
    u32 GapEnd;
    u32 End;
} buffer;

internal void InitBuffer(buffer *Buffer, i32 GapSize) {
    Buffer->Data     = AllocateMemory(GapSize);//HeapAlloc(GetProcessHeap(), 0, GapSize);
    Buffer->GapStart = 0;
    Buffer->GapEnd   = GapSize;
    Buffer->End      = GapSize;
}

internal u32 MoveBufferPositionFoward(buffer *Buffer, u32 Pos) {
    Assert(Pos != Buffer->End);
    Pos++;
    if (Pos == Buffer->GapStart) {
        Pos =  Buffer->GapEnd; 
    }
    return Pos;
}

internal void ShiftGapToPos(buffer *Buffer, u32 Pos) {
    u32 GapSize = Buffer->GapEnd - Buffer->GapStart;
    if (Pos < Buffer->GapStart) {
        u32 Delta = Buffer->GapStart - Pos;
        Buffer->GapStart -= Delta;
        Buffer->GapEnd   -= Delta;
        MoveMemory_(Buffer->Data + Buffer->GapEnd, Buffer->Data + Buffer->GapStart, Delta);
    }
    else
    if (Pos > Buffer->GapStart) {
        u32 Delta = Pos - Buffer->GapStart;
        MoveMemory_(Buffer->Data + Buffer->GapStart, Buffer->Data + Buffer->GapEnd, Delta);
        Buffer->GapStart += Delta;
        Buffer->GapEnd   += Delta;
    }
}

internal void EnsureGapSize(buffer *Buffer, u32 Min) {
    u32 GapSize = Buffer->GapEnd - Buffer->GapStart;
    if (GapSize < Min) {
        ShiftGapToPos(Buffer, Buffer->End - GapSize);
        u32 NewBufferEnd  = 2 * Buffer->End;
        Buffer->Data      = ReallocateMemory(Buffer->Data, Buffer->End, NewBufferEnd);//HeapReAlloc(GetProcessHeap(), 0, Buffer->Data, NewBufferEnd);
        Buffer->GapEnd    = NewBufferEnd;
        Buffer->End       = NewBufferEnd;
    }
    
}

internal void InsertChar(buffer *Buffer, u32 Pos, c8 Char) {
    EnsureGapSize(Buffer, 1);
    ShiftGapToPos(Buffer, Pos);
    Buffer->Data[Buffer->GapStart] = Char;
    Buffer->GapStart++;
}

internal u32 CopyLineFromBuffer(buffer *Buffer, c8 *Dest, u32 DestSize, u32 Pos) {
    for (u32 i = 0; i < DestSize && Pos < Buffer->End; i++) {
        c8  Char = Buffer->Data[Pos];
        if (Char == '\n') {
            break;
        }

       *Dest++ = Char;
        Pos = MoveBufferPositionFoward(Buffer, Pos);
    }
    return Pos;
}
#endif
