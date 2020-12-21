#ifndef GAME_H
#define GAME_H

typedef struct _game_offscreen_buffer {
    void *Mem;
    int32 Width;
    int32 Height;
    int32 Pitch;
} game_offscreen_buffer;

void GameUpdateAndRender(game_offscreen_buffer *Buffer);

#endif//GAHE_H