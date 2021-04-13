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
    return (iv2){x, y};
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
    return (rv2){x, y};
}

typedef struct _color {
    r32 r, g, b, a;
} color;
finginline  color HexToColor(u32 Hex) {
    return (color){((Hex >> 24) & 0xFF)/255.f,
                   ((Hex >> 16) & 0xFF)/255.f,
                   ((Hex >>  8) & 0xFF)/255.f,
                   ((Hex >>  0) & 0xFF)/255.f};
}

typedef union _bcolor {
    u32 rgba;
    struct {
        u8 r, g, b, a;
    };
} bcolor;

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
    return (c32){(z.a * w.a), (z.b * w.b)};
}

inline c32 ExpC32(c32 z) {
    return (c32){(Exp(z.a) * Cos(z.b)), (Exp(z.a) * Sin(z.b))};
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

typedef struct _recti32 {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
} recti32;

typedef struct _rectf32 {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
} rectf32;

typedef struct _rectf {
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
} rectf;
finginline rectf rectf_(r32 x, r32 y, r32 w, r32 h) {
    return (rectf){x, y, w, h};
}

internal b32 IsInsideRect(rv2 Pos, rectf32 Rect) {
    return (Pos.x > Rect.x && Pos.x < Rect.x + Rect.w) && (Pos.y < Rect.y && Pos.y > Rect.y - Rect.h);
}

#endif
