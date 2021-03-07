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

typedef char c8;

typedef float  r32;
typedef double r64;
typedef r32 f32;
typedef r64 f64;

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define Kilobytes(Value) ((Value)* 1024)
#define Megabytes(Value) (Kilobytes(Value)* 1024)
#define Gigabytes(Value) (Megabytes(Value)* 1024)
#define Terabytes(Value) (Gigabytes(Value)* 1024)

inline u32 SafeTruncateU64(u64 Value) {
    u32 Result = 0;
    if (Value <= 0xFFFFFFFF) Result = (u32)Value;
    return Result;
}

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

#endif//LINGO_H
