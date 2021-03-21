#include "lingo.h"
#include "app.h"

#include "maths.h"
#include "platform.h"
#include "memory.h"
#include "opengl.h"

#include "graphics.h"
#include "fonts.h"

typedef struct _buffer {
    c8 *Data;
    u32 GapStart;
    u32 GapEnd;
    u32 End;
} buffer;

internal CreateBuffer(buffer *Buf, u32 GapSize) {
    Buf->Data     = AllocateMemory(GapSize);
    Buf->GapStart = 0;
    Buf->GapEnd   = GapSize;
    Buf->End      = 0;
}

typedef struct _app_state {
    font   RobotoMono;
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

    State->RobotoMono = LoadFont("roboto_mono.ttf", 400, 32);
    InitBuffer(&State->Buffer, 2);
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

    rv2 TextPos = Rv2(State->RobotoMono.Size/2, State->RobotoMono.Size);
    gDrawTextLen(State->RobotoMono, State->Buffer.Data, State->Buffer.GapStart,
                 TextPos, State->RobotoMono.Size, 0, 0, Color4f(1, 1, 1, 1));
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
