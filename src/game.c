#include "game.h"

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    uint8 *Row = (uint8 *)Buffer->Mem;
    for (int Y = 0;Y < Buffer->Height; Y++) {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer->Width; X++) {
            // memory Order: BB GG RR XX
            // 0xXXRRGGBB
            uint8 Blue = X + BlueOffset;
            uint8 Green = Y + GreenOffset;
            *Pixel++ = ((Green << 8) | Blue );
        }
        Row += Buffer->Pitch;
    }
}

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer) {
    RenderWeirdGradient(Buffer, 0, 0);
}