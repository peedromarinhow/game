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

#include <stdint.h>
#include <math.h>

#define internal         static
#define global           static
#define local_persistent static

#define PI32 3.14159265359f

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32   bool32;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float  real32;
typedef double real64;

#if BUILD_SLOW
#   define Assert(Expression) if (!(Expression)) { *(int *)0 = 0; }
#else
#   define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)* 1024)
#define Megabytes(Value) (Kilobytes(Value)* 1024)
#define Gigabytes(Value) (Megabytes(Value)* 1024)
#define Terabytes(Value) (Gigabytes(Value)* 1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32 SafeTruncateUInt64(uint64 Value)
{
    uint32 Result = 0;
    if (Value <= 0xFFFFFFFF) Result = (uint32)Value;
    return Result;
}

#if BUILD_INTERNAL

typedef struct DEBUG_read_file_result
{
    uint64 ContentsSize;
    void *Contents;
} DEBUG_read_file_result;

// pass these as pointers on game_memory
//  and make a snippet for these function pointer macros
#define DEBUG_PLATFORM_FREE_ENTIRE_FILE(name) void name(void *Memory)
typedef DEBUG_PLATFORM_FREE_ENTIRE_FILE(DEBUG_platform_free_entire_file);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) DEBUG_read_file_result name(char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUG_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *Filename, uint64 Size, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUG_platform_write_entire_file);

#endif

typedef struct _game_video_buffer
{
    void *Memory;
    int32 Width;
    int32 Height;
    int32 Pitch;
    int32 BytesPerPixel;
} game_video_buffer;

typedef struct _game_sound_buffer
{
    int32 SamplesPerSecond;
    int32 SampleCount;
    int16 *Samples;
} game_sound_buffer;

typedef struct _game_button_state
{
    int32 HalfTransitionCount;
    bool32 EndedDown;
} game_button_state;

typedef struct _game_controller_input
{
    bool32 IsConnected;
    bool32 IsAnalog;
    real32 StickAverageX;
    real32 StickAverageY;

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
} game_controller_input;

typedef struct _game_input
{
    game_controller_input Controllers[5];
} game_input;

inline game_controller_input *GetController(game_input *Input, uint32 ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return Result;
}

typedef struct _game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void  *PermanentStorageBytes;   //note
                                    //  required to be cleared to zero
    uint64 TransientStorageSize;
    void  *TransientStorageBytes;   //note
                                    //  required to be cleared to zero
    DEBUG_platform_free_entire_file  *DEBUGPlatformFreeEntireFile;
    DEBUG_platform_read_entire_file  *DEBUGPlatformReadEntireFile;
    DEBUG_platform_write_entire_file *DEBUGPlatformWriteEntireFile;

} game_memory;

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_video_buffer *VideoBuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{}

//note
//  at the moment, this funcion should be very fast, < 1ms or so
#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_sound_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub)
{}

typedef struct _game_state
{
    int32 ToneFrequency;

    int32 GreenOffset;
    int32 BlueOffset;

    real32 SineT;

    int32 PlayerX;
    int32 PlayerY;

    real32 JumpT;
} game_state;

#endif//GAHE_H