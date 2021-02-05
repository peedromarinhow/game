#ifndef APP_H
#define APP_H

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
#define Assert(Expression) if (!(Expression)) { *(i32 *)0 = 0; }
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

// pass these as pointers on app_memory
//  and make a snippet for these function pointer macros
#define DEBUG_PLATFORM_FREE_ENTIRE_FILE(Name) \
    void Name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_ENTIRE_FILE(debug_platform_free_entire_file);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(Name) \
    debug_read_file_result Name(thread_context *Thread, char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(Name) \
    b32 Name(thread_context *Thread, char *Filename, u64 Size, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
#endif



typedef struct _app_video_buffer
{
    void *Memory;
    i32 Width;
    i32 Height;
    i32 Pitch;
    i32 BytesPerPixel;
}
app_video_buffer;

typedef struct _app_sound_buffer
{
    i32 SamplesPerSecond;
    i32 SampleCount;
    i16 *Samples;
}
app_sound_buffer;

typedef struct _app_button_state
{
    i32 HalfTransitionCount;
    b32 EndedDown;
}
app_button_state;

typedef struct _app_controller_input
{
    b32 IsConnected;
    b32 IsAnalog;
    r32 StickAverageX;
    r32 StickAverageY;
    i32 MouseX;
    i32 MouseY;
    i32 MouseZ;


    app_button_state MouseButtons[5];

    union
    {
        app_button_state Buttons[12];
        struct
        {
            app_button_state MoveUp;
            app_button_state MoveDown;
            app_button_state MoveLeft;
            app_button_state MoveRight;

            app_button_state ActionUp;
            app_button_state ActionDown;
            app_button_state ActionLeft;
            app_button_state ActionRight;

            app_button_state LeftShoulder;
            app_button_state RightShoulder;

            app_button_state Start;
            app_button_state Back;

            // all buttons should be added above this terminator button

            app_button_state Terminator;
        };
    };
}
app_controller_input;

typedef struct _app_input
{
    r32 dtForFrame;
    app_controller_input Controllers[5];
}
app_input;

inline app_controller_input *GetController(app_input *Input, u32 ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    return &Input->Controllers[ControllerIndex];
}

typedef struct _app_memory
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
app_memory;

#define APP_UPDATE_AND_RENDER(Name)                       \
    void Name(thread_context *Thread, app_memory *Memory, \
              app_input *Input, app_video_buffer *VideoBuffer)
typedef APP_UPDATE_AND_RENDER(app_update_and_render);

//note
//  at the moment, this funcion should be very fast, < 1ms or so
#define APP_GET_SOUND_SAMPLES(Name)                       \
    void Name(thread_context *Thread, app_memory *Memory, \
              app_sound_buffer *SoundBuffer)
typedef APP_GET_SOUND_SAMPLES(app_get_sound_samples);

typedef struct _loaded_bitmap
{
    i32 Width;
    i32 Height;
    u32 *Pixels;
}
loaded_bitmap;

typedef struct _app_state
{
    r32 FontScale;
}
app_state;

#endif//GAHE_H

int main(int Argc, char **Argv)
{
    return 0;
}
