#include "game.h"

internal void OutputSound(game_sound_buffer *SoundBuffer, int32 ToneFrequency) {
    local_persistent real32 SineT;
    int16 ToneVolume = 3000;
    int32 WavePeriod = SoundBuffer->SamplesPerSecond / ToneFrequency;

    int16* SampleOut = SoundBuffer->Samples;
    for (int32 SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
        real32 SineValue = sinf(SineT);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        SineT += 2.0f * PI32 / (real32)WavePeriod;
    }
}

internal void RenderWeirdGradient(game_video_buffer *Buffer, int32 BlueOffset, int32 GreenOffset) {
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

internal void GameUpdateAndRender(game_input *Input, game_sound_buffer *SoundBuffer, game_video_buffer *VideoBuffer) {
    int32 ToneFrequency = 522;
    local_persistent int32 GreenOffset = 0;
    local_persistent int32 BlueOffset = 0;

    game_controller_input *Input0 = &Input->Controllers[0];
    if (Input0->IsAnalog) {
        // analog movement tuning
        // I don't have a controller to test this
        ToneFrequency = 256 + (int32)(128.0f * Input0->EndX);
        BlueOffset += (int32)4.0f * Input0->EndY;
    }
    else {
        // digital movement tuning
    }

    if (Input0->Down.EndedDown) {
        // can't test this either
        GreenOffset += 1;
    }

    OutputSound(SoundBuffer, ToneFrequency);
    RenderWeirdGradient(VideoBuffer, GreenOffset, BlueOffset);
}