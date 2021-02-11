#ifndef LINGO_H
#define LINGO_H

#include <stdint.h>

#define internal     static
#define global       static
#define localpersist static

#define PI32 3.14159265359f

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32     b32;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  r32;
typedef double r64;
typedef r32 f32;
typedef r64 f64;

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define Bytes(n)      (n)
#define Kilobytes(n)  (n << 10)
#define Megabytes(n)  (n << 20)
#define Gigabytes(n)  (((u64)n) << 30)
#define Terabytes(n)  (((u64)n) << 40)

#if BUILD_SLOW
    #define Assert(Expression) if (!(Expression)) { *(i32 *)0 = 0; }
#else
    #define Assert(Expression)
#endif

internal int StringLenght(char *String) {
    int Count = 0;
    while (*String++)
        Count++;
    return Count;
}

#endif