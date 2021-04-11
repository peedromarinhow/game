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
        i32 w;
        i32 h;
    };
    i32 Comps[2];
} iv2;
finginline iv2 iv2_(i32 x, i32 y) {
    iv2    Res = {x, y};
    return Res;
}

typedef union _rv2 {
    struct {
        r32 x;
        r32 y;
    };
    struct {
        r32 w;
        r32 h;
    };
    r32 Comps[2];
} rv2;
finginline rv2 rv2_(r32 x, r32 y) {
    rv2    Res = {x, y};
    return Res;
}

typedef struct _color {
    r32 r;
    r32 g;
    r32 b;
    r32 a;
} color;

finginline color HexToColor(u32 Hex) {
    color Res = {((Hex >> 24) & 0xFF)/255.f,
                 ((Hex >> 16) & 0xFF)/255.f,
                 ((Hex >>  8) & 0xFF)/255.f,
                 ((Hex >>  0) & 0xFF)/255.f};
    return Res;
}

typedef struct _color4b {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} color4b;

typedef union _c32 {
    struct {
        r32 Real;
        r32 Imaginary;
    };
    struct {
        r32 a;
        r32 b;
    };
    r32 Comps[2];
} c32;

inline c32 MulC32(c32 z, c32 w) {
    c32    Res = {(z.a * w.a), (z.b * w.b)};
    return Res;
}

inline c32 ExpC32(c32 z) {
    c32    Res = {(Exp(z.a) * Cos(z.b)), (Exp(z.a) * Sin(z.b))};
    return Res;
}

inline c32 LogC32(c32 z) {
    r32 c = Sqrt(z.a*z.a + z.b*z.b);
    r32 d = Arctan(z.b/z.a);
        c = Log(c);
    if (z.a < 0) d += PI32;
    c32    Res = {c, d};
    return Res;
}

inline c32 PowC32(c32 z, c32 p) {
    return ExpC32(MulC32(p, LogC32(z)));
}

typedef struct _recti32 {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
} recti32;

typedef struct _rectf32 {
    union {
        struct { rv2 Pos; };
        struct {
            f32 x;
            f32 y;
        };
    };
    union {
        struct { rv2 Dim; };
        struct {
            f32 w;
            f32 h;
        };
    };
} rectf32;
finginline rectf32 rectf32_(r32 x, r32 y, r32 w, r32 h) {
    rectf32 Res = {x, y, w, h};
    return Res;
}

#endif
