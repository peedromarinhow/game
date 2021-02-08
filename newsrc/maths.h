#include "lingo.h"

#ifndef MATHS_H
#define MATHS_H

typedef union _iv2 {
    struct {
        i32 X;
        i32 Y;
    };

    struct {
        i32 Width;
        i32 Height;
    };

    i32 Components[2];
} iv2;

#endif