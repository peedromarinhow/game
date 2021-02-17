#ifndef MEMORY_H
#define MEMORY_H

#include "lingo.h"

typedef struct _app_memory {
    void *Contents;
    u32   Size;
} app_memory;

typedef struct _memory_arena {
    u32 Size;
    u32 Used;
    u8 *Base;
} memory_arena;

internal void InitializeArena(memory_arena *Arena, u32 Size, void *Base) {
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushStructToArena(Arena, type) (type *)_PushStructToArena((Arena), sizeof(type))
internal void *_PushStructToArena(memory_arena *Arena, u32 Size) {
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return Result;
}

#endif//MEMORY_H
