#include "lingo.h"

#ifndef MATHS_H
#define MATHS_H

typedef union _iv2 {
    struct {
        i32 x;
        i32 y;
    };

    struct {
        i32 Width;
        i32 Height;
    };

    i32 Components[2];
} iv2;

typedef union _rv2 {
    struct {
        r32 x;
        r32 y;
    };

    // struct {
    //     r32 Width;
    //     r32 Height;
    // };

    r32 Components[2];
} rv2;

typedef union _c32 {
    struct {
        r32 Real;
        r32 Imaginary;
    };
    struct {
        r32 a;
        r32 b;
    };
    r32 Components[2];
} c32;

inline c32 MulC32(c32 z, c32 w) {
    return {(z.a * w.a), (z.b * w.b)};
}

inline c32 ExpC32(c32 z) {
    return {(Exp(z.a) * Cos(z.b)), (Exp(z.a) * Sen(z.b))};
}

inline c32 LogC32(c32 z) {
    return {0, 0};
}

inline c32 PowC32(c32 z, c32 p) {
    return ExpC32(MulC32(p, LogC32(z)));
}

#endif