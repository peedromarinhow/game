#include "game.h"

// internal
void RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    uint8_t *Row = (uint8_t *)Buffer->Mem;
    for (int Y = 0;Y < Buffer->Height; Y++) {
        uint32_t *Pixel = (uint32_t *)Row;
        for (int X = 0; X < Buffer->Width; X++) {
            // memory Order: BB GG RR XX
            // 0xXXRRGGBB
            uint8_t Blue = X + BlueOffset;
            uint8_t Green = Y + GreenOffset;
            *Pixel++ = ((Green << 8) | Blue );
        }
        Row += Buffer->Pitch;
    }
}

// internal
void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
    RenderWeirdGradient(Buffer, XOffset, YOffset);
}