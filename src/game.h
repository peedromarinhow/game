#ifndef GAME_H
#define GAME_H

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef struct _game_video_buffer {
    void *Mem;
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
    bool32 IsAnalog;

    real32 StartX;
    real32 MinX;
    real32 MaxX;
    real32 EndX;

    real32 StartY;
    real32 MinY;
    real32 MaxY;
    real32 EndY;

    union {
        game_button_state Buttons[6];
        struct {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
} game_controller_input;

typedef struct _game_input {
    game_controller_input Controllers[4];
} game_input;

internal void GameUpdateAndRender(game_input *Input, game_sound_buffer *SoundBuffer, game_video_buffer *VideoBuffer);

#endif//GAHE_H