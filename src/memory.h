#ifndef MEMORY_H
#define MEMORY_H

#include "lingo.h"
// #include "platform.h"

typedef struct _memory_arena {
    u64 MaxSize;
    u8 *Base;
    u64 Used;
} memory_arena;

internal memory_arena InitializeArena(u64 MaxSize, void *Base) {
    memory_arena Arena = {
        .MaxSize = MaxSize,
        .Base    = Base,
        .Used    = 0
    };
    return Arena;
}

internal void *PushToArena(memory_arena *Arena, u64 Size) {
    void *Memory = 0;
    if ((Arena->Used + Size) < Arena->MaxSize) {
        Memory = (u8 *)Arena->Base + Arena->Used;
        Arena->Used += Size;
    }
    return Memory;
}

internal void PopFromArena(memory_arena *Arena, u64 Size) {
    if (Size < Arena->Used) {
        Size = Arena->Used;
    }
    Arena->Used -= Size;
}

internal void ClearArena(memory_arena *Arena) {
    PopFromArena(Arena, Arena->Used);
}

#endif//MEMORY_H
