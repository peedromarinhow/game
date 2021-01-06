#include "game.h"

internal void OutputSound(game_sound_buffer *SoundBuffer, int32 ToneFrequency)
{
    local_persistent real32 SineT;
    int16 ToneVolume = 3000;
    int32 WavePeriod = SoundBuffer->SamplesPerSecond / ToneFrequency;

    int16* SampleOut = SoundBuffer->Samples;
    for (int32 SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        real32 SineValue = sinf(SineT);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        SineT += 2.0f * PI32 / (real32)WavePeriod;
    }
}

internal void RenderWeirdGradient(game_video_buffer *Buffer, int32 BlueOffset, int32 GreenOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;
    for (int Y = 0;Y < Buffer->Height; Y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer->Width; X++)
        {
            // memory Order: BB GG RR XX
            // 0xXXRRGGBB
            //uint8 Blue = (uint8)(X + BlueOffset);
            //uint8 Green = (uint8)(Y + GreenOffset);
            *Pixel++ = 0xFFFF0000;//((Green << 8) | Blue );
        }
        Row += Buffer->Pitch;
    }
}

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_video_buffer *VideoBuffer)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *State = (game_state *)Memory->PermanentStorageBytes;
    if (!Memory->IsInitialized)
    {
        char *Filename = __FILE__;
        DEBUG_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
        if (File.Contents)
        {
            DEBUGPlatformWriteEntireFile("D:\\code\\game\\data\\test.out", File.ContentsSize, File.Contents);
            DEBUGPlatformFreeEntireFile(File.Contents);
        }

        State->ToneFrequency = 261;

        Memory->IsInitialized = true;
    }

    for (int32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog)
        {
            // analog movement tuning
            // I don't have a controller to test this
            State->ToneFrequency = 256 + (int32)(128.0f * Controller->StickAverageX);
            State->BlueOffset += (int32)(4.0f * Controller->StickAverageY);
        }
        else
        {
            if (Controller->MoveLeft.EndedDown)
            {
                State->GreenOffset += 10;
            }
            if (Controller->MoveRight.EndedDown)
            {
                State->GreenOffset -= 10;
            }
            if (Controller->MoveUp.EndedDown)
            {
                State->BlueOffset -= 10;
            }
            if (Controller->MoveDown.EndedDown)
            {
                State->BlueOffset += 10;
            }
        }
    }
    RenderWeirdGradient(VideoBuffer, State->GreenOffset, State->BlueOffset);
}

internal void GameGetSoundSamples(game_memory *Memory, game_sound_buffer *SoundBuffer)
{
    game_state *State = (game_state *)Memory->PermanentStorageBytes;
    OutputSound(SoundBuffer, State->ToneFrequency);
}