#ifndef GAME_H
#define GAME_H

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

void GameUpdateAndRender(game_sound_buffer *SoundBuffer, game_video_buffer *Buffer, int32 GreenOffset, int32 BlueOffset);
void OutputSound(game_sound_buffer *SoundBuffer);

#endif//GAHE_H