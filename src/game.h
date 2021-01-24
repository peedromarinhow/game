#ifndef GAME_H
#define GAME_H

/*
    BUILD_INTERNAL:
        0: for public release
        1: for development

    BUILD_SLOW:
        0: no slow code allowed
        1: slow code allowed
 */

// note
//  using CRT for now
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

#define internal static
#define global   static
#define localper static

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

#if BUILD_SLOW
#define Assert(Expression) if (!(Expression)) { *(int *)0 = 0; }
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)* 1024)
#define Megabytes(Value) (Kilobytes(Value)* 1024)
#define Gigabytes(Value) (Megabytes(Value)* 1024)
#define Terabytes(Value) (Gigabytes(Value)* 1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline u32 SafeTruncateUInt64(u64 Value)
{
    u32 Result = 0;
    if (Value <= 0xFFFFFFFF) Result = (u32)Value;
    return Result;
}

typedef struct _thread_context
{
    int PlaceHolder;
}
thread_context;

#if BUILD_INTERNAL

typedef struct debug_read_file_result
{
    u64 ContentsSize;
    void *Contents;
}
debug_read_file_result;

// pass these as pointers on game_memory
//  and make a snippet for these function pointer macros
#define DEBUG_PLATFORM_FREE_ENTIRE_FILE(name) \
    void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_ENTIRE_FILE(debug_platform_free_entire_file);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) \
    debug_read_file_result name(thread_context *Thread, char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) \
    b32 name(thread_context *Thread, char *Filename, u64 Size, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif

typedef struct _game_video_buffer
{
    void *Memory;
    i32 Width;
    i32 Height;
    i32 Pitch;
    i32 BytesPerPixel;
}
game_video_buffer;

typedef struct _game_sound_buffer
{
    i32 SamplesPerSecond;
    i32 SampleCount;
    i16 *Samples;
}
game_sound_buffer;

typedef struct _game_button_state
{
    i32 HalfTransitionCount;
    b32 EndedDown;
}
game_button_state;

typedef struct _game_controller_input
{
    b32 IsConnected;
    b32 IsAnalog;
    r32 StickAverageX;
    r32 StickAverageY;
    i32 MouseX;
    i32 MouseY;
    i32 MouseZ;


    game_button_state MouseButtons[5];

    union
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Start;
            game_button_state Back;

            // all buttons should be added above this terminator button

            game_button_state Terminator;
        };
    };
}
game_controller_input;

typedef struct _game_input
{
    r32 dtForFrame;
    game_controller_input Controllers[5];
}
game_input;

inline game_controller_input *GetController(game_input *Input, u32 ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    return &Input->Controllers[ControllerIndex];
}

typedef struct _game_memory
{
    b32 IsInitialized;
    u64 PermanentStorageSize;
    void  *PermanentStorageBytes;   //note
                                    //  required to be cleared to zero
    u64 TransientStorageSize;
    void  *TransientStorageBytes;   //note
                                    //  required to be cleared to zero
    debug_platform_free_entire_file  *DEBUGPlatformFreeEntireFile;
    debug_platform_read_entire_file  *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;

}
game_memory;

#define GAME_UPDATE_AND_RENDER(name)                       \
    void name(thread_context *Thread, game_memory *Memory, \
              game_input *Input, game_video_buffer *VideoBuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

//note
//  at the moment, this funcion should be very fast, < 1ms or so
#define GAME_GET_SOUND_SAMPLES(name)                       \
    void name(thread_context *Thread, game_memory *Memory, \
              game_sound_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

typedef struct _game_state
{
    r32 PlayerX;
    r32 PlayerY;
}
game_state;

#endif//GAHE_H
