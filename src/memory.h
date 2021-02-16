#ifndef MEMORY_H
#define MEMORY_H

#include "lingo.h"

typedef struct _app_memory {
    void *Contents;
    u64   Size;
} app_memory;

typedef struct _arena {
    void *Base;
    u64   Max;
    u64   AllocPos;
    u64   CommitPos;
} arena;

#endif//MEMORY_H