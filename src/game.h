#ifndef GAME_H
#define GAME_H

/*
 *  BUILD_INTERNAL:
 *      0: for public release
 *      1: for development
 * 
 *  BUILD_SLOW:
 *      0: no slow code allowed
 *      1: slow code allowed
 */

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

inline uint32 SafeTruncateUInt64(uint64 Value) {
    uint32 Result = 0;
    if (Value <= 0xFFFFFFFF) Result = (uint32)Value;
    return Result;
}

#if BUILD_INTERNAL

typedef struct _debug_read_file_result {
    uint64 ContentsSize;
    void *Contents;
} debug_read_file_result;

internal void DEBUGPlatformFreeEntireFile(void *Memory);
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal bool32 DEBUGPlatformWriteEntireFile(char* Filename, uint64 Size, void *Memory);

#else
#endif

typedef struct _game_video_buffer {
    void *Memory;
    int32 Width;
    int32 Height;
    int32 Pitch;
} game_video_buffer;

typedef struct _game_sound_buffer {
    int32 SamplesPerSecond;
    int32 SampleCount;
    int16 *Samples;
} game_sound_buffer;

typedef struct _game_button_state {
    int32 HalfTransitionCount;
    bool32 EndedDown;
} game_button_state;

typedef struct _game_controller_input {
    bool32 IsConnected;
    bool32 IsAnalog;
    real32 StickAverageX;
    real32 StickAverageY;

    union {
        game_button_state Buttons[12];
        struct {
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

typedef struct _game_input {
    game_controller_input Controllers[5];
} game_input;

inline game_controller_input *GetController(game_input *Input, uint32 ControllerIndex) {
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return Result;
}

typedef struct _game_memory {
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void  *PermanentStorageBytes;   // note: required to be cleared to zero

    uint64 TransientStorageSize;
    void  *TransientStorageBytes;   // note: required to be cleared to zero
} game_memory;

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_sound_buffer *SoundBuffer, game_video_buffer *VideoBuffer);

typedef struct _game_state {
    int32 ToneFrequency;
    int32 GreenOffset;
    int32 BlueOffset;
} game_state;

#endif//GAHE_H