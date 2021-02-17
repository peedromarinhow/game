#ifndef MATHS_H
#define MATHS_H

#include <math.h>
//note: CRT because implementing the stuff is going to take too long

#define Exp    expf
#define Sin    sinf
#define Cos    cosf
#define Arctan atanf
#define Log    logf
#define Sqrt   sqrtf

#include "lingo.h"

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
    return (c32){(z.a * w.a), (z.b * w.b)};
}

inline c32 ExpC32(c32 z) {
    return (c32){(Exp(z.a) * Cos(z.b)), (Exp(z.a) * Sin(z.b))};
}

inline c32 ExpITheta(r32 Theta) {
    return (c32){Cos(Theta), Sin(Theta)};
}

inline c32 LogC32(c32 z) {
    r32 c = Sqrt(z.a*z.a + z.b*z.b);
    r32 d = Arctan(z.b/z.a);
        c = Log(c);
    if (z.a < 0) d += PI32;
    return (c32){c, d};
}

inline c32 PowC32(c32 z, c32 p) {
    return ExpC32(MulC32(p, LogC32(z)));
}

#endif
