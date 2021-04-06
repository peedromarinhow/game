#include "lingo.h"
#include "app.h"

#include "maths.h"
#include "platform.h"
#include "memory.h"
#include "opengl.h"

#include "graphics.h"
#include "buffer.h"

typedef struct _app_state {
    keymap *Keymap;  
    buffer *Buffer;
    font    RobotoMono;
    font    Roboto;
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

    State->Keymap     = CreateDeafaultKeymap();
    State->Buffer     = CreateBuffer(2);
    State->RobotoMono = LoadFont("roboto_mono.ttf", 400, 32);
    State->Roboto     = LoadFont("roboto.ttf", 400, 32);
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

    State->RobotoMono = LoadFont("roboto_mono.ttf", 400, 32);
    State->Roboto     = LoadFont("roboto.ttf", 400, 32);
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;

    Clear(ORIGIN_CENTERED, p->WindowDimensions, HexToColor(0x20202000));
    DrawRect(ORIGIN_CENTERED, rv2_(100, 100), rv2_(100, 100), HexToColor(0xFAFAFAFF), 0, HexToColor(0x606060FF));
    DrawLine(rv2_(0, 0), p->MousePos, 4, HexToColor(0x4040FAFF));
    DrawText_(&State->RobotoMono, "a\n\tb\rcdef", rv2_(200, 600), 32, 0, 0, HexToColor(0xFAFAFAFF));
    DrawBuffer(rv2_(16, p->WindowDimensions.y - 32), State->Buffer, &State->RobotoMono, State->RobotoMono.Size);
    
    key Key = KEY_NONE;
    if (p->kChar)
        Key = KEY_CHAR;
    if (p->kDelete)
        Key = KEY_DEL;
    if (p->kBack)
        Key = KEY_BACK;
    if (p->kLeft)
        Key = KEY_LEFT;
    if (p->kRight)
        Key = KEY_RIGHT;
    if (p->kHome)
        Key = KEY_HOME;
    if (p->kEnd)
        Key = KEY_END;
    if (p->kReturn)
        Key = KEY_RETURN;
    
    State->Keymap->Commands[Key].Func((command_context){State->Buffer, p->KeyboardChar});
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}

#if 0

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

internal void BufferShiftGapToPos(buffer *Buffer, u32 Pos) {
    Buffer->Cursor.GapStart = Pos;
}

void InsertChar(buffer *Buffer, u32 Pos, c8 Char) {
    if (Buffer->Cursor.Pos >= 0) {
        Buffer->Data[Buffer->Cursor.GapStart] = Char;
        Buffer->Cursor.Pos = Buffer->Cursor.GapStart + 1;
        BufferShiftGapToPos(Buffer, Buffer->Cursor.GapStart + 1);
    }
}

void RemoveChar(buffer *Buffer) {
    if (Buffer->Cursor.Pos > 0) {
        Buffer->Data[Buffer->Cursor.GapStart] = 0;
        Buffer->Cursor.Pos = Buffer->Cursor.GapStart - 1;
        BufferShiftGapToPos(Buffer, Buffer->Cursor.GapStart - 1);
    }
}

    if (p->KeyboardCharCame) {
        if (p->KeyboardChar == '\b')
            RemoveChar(State->Buffer);
        else
        if (p->KeyboardChar == '\r')
            InsertChar(State->Buffer, State->Buffer.Cursor.GapStart, '\n');
        else
        if (p->KeyboardChar == '\t') {
            InsertChar(State->Buffer, State->Buffer.Cursor.GapStart, ' ');
            InsertChar(State->Buffer, State->Buffer.Cursor.GapStart, ' ');
            InsertChar(State->Buffer, State->Buffer.Cursor.GapStart, ' ');
            InsertChar(State->Buffer, State->Buffer.Cursor.GapStart, ' ');
        }
        else
            InsertChar(State->Buffer, State->Buffer.Cursor.GapStart, p->KeyboardChar);
    }

    if (p->dMouseWheel > 0) {
        State->Pos.y += State->RobotoMono.Size/2;
    }
    else
    if (p->dMouseWheel < 0) {
        State->Pos.y -= State->RobotoMono.Size/2;
    }
    else
        State->Pos.y += 0;

    if (p->kLeft && State->Buffer.Cursor.Pos > 1)
        State->Buffer.Cursor.Pos--;
    if (p->kRight && State->Buffer.Cursor.Pos < State->Buffer.Cursor.GapStart)
        State->Buffer.Cursor.Pos++;

typedef struct _buffer {
    c8 *Data;
    u32.Cursor.GapStart;
    u32 Cursor.GapEnd;
    u32 End;
} buffer;

internal void InitBuffer(buffer *Buffer, i32 GapSize) {
    Buffer->Data     = AllocateMemory(GapSize);//HeapAlloc(GetProcessHeap(), 0, GapSize);
    Buffer->Cursor.GapStart = 0;
    Buffer->Cursor.GapEnd   = GapSize;
    Buffer->End      = GapSize;
}

internal u32 MoveBufferPositionFoward(buffer *Buffer, u32 Pos) {
    Assert(Pos != Buffer->End);
    Pos++;
    if (Pos == Buffer->Cursor.GapStart) {
        Pos =  Buffer->Cursor.GapEnd; 
    }
    return Pos;
}

internal void ShiftGapToPos(buffer *Buffer, u32 Pos) {
    u32 GapSize = Buffer->Cursor.GapEnd - Buffer->Cursor.GapStart;
    if (Pos < Buffer->Cursor.GapStart) {
        u32 Delta = Buffer->Cursor.GapStart - Pos;
        Buffer->Cursor.GapStart -= Delta;
        Buffer->Cursor.GapEnd   -= Delta;
        MoveMemory_(Buffer->Data + Buffer->Cursor.GapEnd, Buffer->Data + Buffer->Cursor.GapStart, Delta);
    }
    else
    if (Pos > Buffer->Cursor.GapStart) {
        u32 Delta = Pos - Buffer->Cursor.GapStart;
        MoveMemory_(Buffer->Data + Buffer->Cursor.GapStart, Buffer->Data + Buffer->Cursor.GapEnd, Delta);
        Buffer->Cursor.GapStart += Delta;
        Buffer->Cursor.GapEnd   += Delta;
    }
}

internal void EnsureGapSize(buffer *Buffer, u32 Min) {
    u32 GapSize = Buffer->Cursor.GapEnd - Buffer->Cursor.GapStart;
    if (GapSize < Min) {
        ShiftGapToPos(Buffer, Buffer->End - GapSize);
        u32 NewBufferEnd  = 2 * Buffer->End;
        Buffer->Data      = ReallocateMemory(Buffer->Data, Buffer->End, NewBufferEnd);//HeapReAlloc(GetProcessHeap(), 0, Buffer->Data, NewBufferEnd);
        Buffer->Cursor.GapEnd    = NewBufferEnd;
        Buffer->End       = NewBufferEnd;
    }
    
}

internal void InsertChar(buffer *Buffer, u32 Pos, c8 Char) {
    EnsureGapSize(Buffer, 1);
    ShiftGapToPos(Buffer, Pos);
    Buffer->Data[Buffer->Cursor.GapStart] = Char;
    Buffer->Cursor.GapStart++;
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
