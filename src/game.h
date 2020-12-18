#ifndef GAME_H
#define GAME_H

typedef struct _game_offscreen_buffer {
    void *Mem;
    int   Width;
    int   Height;
    int   Pitch;
} game_offscreen_buffer;

void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

#endif//GAHE_H