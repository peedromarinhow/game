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

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_sound_buffer *SoundBuffer, game_video_buffer *VideoBuffer) {
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *State = (game_state *)Memory->PermanentStorageBytes;
    if (!Memory->IsInitialized) {
        char *Filename = __FILE__;
        debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
        if (File.Contents) {
            DEBUGPlatformWriteEntireFile("D:\\code\\game\\data\\test.out", File.ContentsSize, File.Contents);
            DEBUGPlatformFreeEntireFile(File.Contents);
        }

        State->ToneFrequency = 261;

        Memory->IsInitialized = true;
    }

    game_controller_input *Input0 = &Input->Controllers[0];
    if (Input0->IsAnalog) {
        // analog movement tuning
        // I don't have a controller to test this
        State->ToneFrequency = 256 + (int32)(128.0f * Input0->EndX);
        State->BlueOffset += (int32)4.0f * Input0->EndY;
    }
    else {
        // digital movement tuning
    }

    if (Input0->Down.EndedDown) {
        // can't test this either
        State->GreenOffset += 1;
    }

    OutputSound(SoundBuffer, State->ToneFrequency);
    RenderWeirdGradient(VideoBuffer, State->GreenOffset, State->BlueOffset);
}