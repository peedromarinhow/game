#include "lingo.h"
#include "maths.h"
#include "platform.h"
#include "memory.h"
#include "opengl.h"
#include "app.h"

#include "graphics.h"
#include "fonts.h"

internal void MoveMemory_(c8 *Dest, c8 *Src, u32 Size) {
    c8 *Tmp = AllocateMemory(Size);
    for (u32 i = 0; i < Size; i++) 
        Tmp[i] = Src[i]; 
    for (u32 i = 0; i < Size; i++) 
        Dest[i] = Tmp[i]; 
    FreeMemory((void *)Tmp);
}

internal void CopyMemory_(c8 *Dest, c8 *Src, u32 Size) {
    for (u32 i = 0; i < Size; i++) 
        Dest[i] = Src[i];
}

internal void *ReallocateMemory(void *Ptr, u32 OriginalLen, u32 NewLen) {
    if (NewLen == 0) {
        FreeMemory(Ptr);
        return NULL;
    }
    else
    if (!Ptr) {
        return AllocateMemory(NewLen);
    }
    else
    if (NewLen <= OriginalLen) {
        return Ptr;
    }
    else {
        if((Ptr) && (NewLen > OriginalLen)) {
            void *PtrNew = AllocateMemory(NewLen);
            if (PtrNew) {
                CopyMemory_(PtrNew, Ptr, OriginalLen);
                FreeMemory(Ptr);
            }
            return PtrNew;
        }
    }
    return NULL;
}

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
    InitBuffer(&State->Buffer, 4);
    for (c8 c = 'a'; c < 'z'; c++) {
        InsertChar(&State->Buffer, 0, c);
        InsertChar(&State->Buffer, 0, c + ('A' - 'a'));
    }
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

    gDrawTextLen(State->RobotoMono, State->Buffer.Data, State->Buffer.End,
                 Rv2(16, State->RobotoMono.Size), State->RobotoMono.Size,
                 0, 0, Color4f(1, 1, 1, 1));
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}
