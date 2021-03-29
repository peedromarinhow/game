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
    u32 Cursor;
    u32 GapStart;
    u32 GapEnd;
    u32 End;
} buffer;

internal buffer CreateBuffer(u32 Size) {
    buffer Result = {0}; {
        Result.Data      = AllocateMemory(Size);
        Result.Cursor    = 0;
        Result.GapStart  = Result.Cursor;
        Result.GapEnd    = Size;
        Result.End       = Size;
    }

    return Result;
}

internal void BufferShiftGapToPos(buffer *Buffer, u32 Pos) {
    Buffer->GapStart = Pos;
}

void InsertChar(buffer *Buffer, u32 Pos, c8 Char) {
    if (Buffer->Cursor == Buffer->GapStart) {
        Buffer->Data[Buffer->GapStart] = Char;
        Buffer->Cursor++;
        BufferShiftGapToPos(Buffer, Buffer->GapStart + 1);
    }
}

void RemoveChar(buffer *Buffer) {
    if ((Buffer->GapStart > 0) && (Buffer->Cursor == Buffer->GapStart)) {
        Buffer->Data[Buffer->GapStart] = 0;
        Buffer->Cursor--;
        BufferShiftGapToPos(Buffer, Buffer->GapStart - 1);
    }
}

void gDrawBuffer(buffer *Buffer, font *Font, rv2 Pos, r32 Size, r32 LineSpacing, r32 CharSpacing, color4f Tint) {
    f32 ScaleFactor = 1;//Size/Font->Size;
    f32 CharOffset  = 0;
    f32 LineOffset  = 0;

    r32 TabSize = (Font->Size + Font->Size/2);

    glBindTexture(GL_TEXTURE_2D, Font->Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
    for (u32 i = 0; i < Buffer->GapStart; i++) {
        i32 CodepointByteCount = 0;
        i32 Codepoint = GetNextCodepoint(&Buffer->Data[i], &CodepointByteCount);
        i32 Index     = GetGlyphIndex(*Font, Codepoint);

        if (Codepoint == 0x3f)
            CodepointByteCount = 1;

        i32 OffY      = Font->Chars[Index].OffY;
        i32 OffX      = Font->Chars[Index].OffX;
        rectf32 Rect  = Font->Rects[Index];
        f32 w         = Font->Texture.w;
        f32 h         = Font->Texture.h;

        if (Buffer->Data[i] == '\t') {
            CharOffset += TabSize;
        }
        else
        if (Buffer->Data[i] == '\n') {
            LineOffset += Font->Size + (LineSpacing * ScaleFactor);
            CharOffset = 0;
        }
        else {
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
        }
        
        if (i == Buffer->Cursor - 1) {
            glBegin(GL_LINES); {
                glLineWidth(2);
                r32 x = Pos.x + CharOffset + Rect.w + OffX + 2;
                if (CharOffset == 0)
                    x = Pos.x + OffX + 2;
                glVertex2f(x, Pos.y + LineOffset + Font->Size*.25f);
                glVertex2f(x, Pos.y + LineOffset - Font->Size*.75f);
            } glEnd();
        }

        if (Buffer->Data[i] != '\n') {
            CharOffset += (Font->Chars[Index].Advance * ScaleFactor) +
                          (CharSpacing * ScaleFactor);
        }

        i += (CodepointByteCount - 1);
    }
}

typedef struct _app_state {
    font   RobotoMono;
    file   TestFile;
    rv2    TestFilePos;
    buffer Buffer;
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

    State->RobotoMono  = LoadFont("roboto.ttf", 400, 32);
    State->TestFile    = LoadFile(__FILE__);
    State->TestFilePos = Rv2(16, 32);
    State->Buffer      = CreateBuffer(16);

    InsertChar(&State->Buffer, 1, 0xC3);
    InsertChar(&State->Buffer, 0, 0x80);
    InsertChar(&State->Buffer, 3, 'B');
    InsertChar(&State->Buffer, 2, 'E');
    InsertChar(&State->Buffer, 5, 'R');
    InsertChar(&State->Buffer, 4, 'A');
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
    gBegin(Rv2(0, 0), p->WindowDimensions, Color4f(0.2f, 0.2f, 0.2f, 1));
    
    if (p->KeyboardCharacterCame) {
        if (p->KeyboardCharacter == '\b')
            RemoveChar(&State->Buffer);
        else
        if (p->KeyboardCharacter == '\r')
            InsertChar(&State->Buffer, State->Buffer.GapStart, '\n');
        else
        if (p->KeyboardCharacter == '\t') {
            InsertChar(&State->Buffer, State->Buffer.GapStart, ' ');
            InsertChar(&State->Buffer, State->Buffer.GapStart, ' ');
            InsertChar(&State->Buffer, State->Buffer.GapStart, ' ');
            InsertChar(&State->Buffer, State->Buffer.GapStart, ' ');
        }
        else
            InsertChar(&State->Buffer, State->Buffer.GapStart, p->KeyboardCharacter);
    }

    if (p->dMouseWheel > 0) {
        State->TestFilePos.y += State->RobotoMono.Size/2;
    }
    else
    if (p->dMouseWheel < 0) {
        State->TestFilePos.y -= State->RobotoMono.Size/2;
    }
    else
        State->TestFilePos.y += 0;

    if (p->kLeft && State->Buffer.Cursor > 1)
        State->Buffer.Cursor--;
    if (p->kRight && State->Buffer.Cursor < State->Buffer.GapStart)
        State->Buffer.Cursor++;

    gDrawText(&State->RobotoMono, "Ленивый рыжий кот", State->TestFilePos,
               State->RobotoMono.Size, 0, 0, Color4f(1, 1, 1, 1), NULL);

    // gDrawBuffer(&State->Buffer, &State->RobotoMono, State->TestFilePos, State->RobotoMono.Size, 0, 0, Color4f(1, 1, 1, 1));
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
