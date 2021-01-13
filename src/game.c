#include "game.h"

void OutputSineWave(game_sound_buffer *SoundBuffer, int32 ToneFrequency)
{
    int16 ToneVolume = 3000;
    int32 WavePeriod = SoundBuffer->SamplesPerSecond / ToneFrequency;

    int16* SampleOut = SoundBuffer->Samples;
    for (int32 SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
#if 0   
        real32 SineValue = sinf(GameState->SineT);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
#if 0
        GameState->SineT += 2.0f * PI32 / (real32)WavePeriod;
        if (GameState->SineT > (2.0f * PI32))
        {
            GameState->SineT -= 2.0f * PI32;
        }
#endif
    }
}

internal int32 RoundR32toI32(real32 Real32)
{
    int32 Result = (int32)(Real32 + 0.5f);
    return Result;
}

void DrawRectangle(game_video_buffer *Buffer, 
                   real32 RealMinX, real32 RealMinY,
                   real32 RealMaxX, real32 RealMaxY,
                   uint32 Color)
{
    int32 MinX = RoundR32toI32(RealMinX);
    int32 MinY = RoundR32toI32(RealMinY);
    int32 MaxX = RoundR32toI32(RealMaxX);
    int32 MaxY = RoundR32toI32(RealMaxY);

    if (MinX < 0)
        MinX = 0;

    if (MinY < 0)
        MinY = 0;

    if (MaxX > Buffer->Width)
        MaxX = Buffer->Width;

    if (MaxY > Buffer->Height)
        MaxY = Buffer->Height;

    uint8 *Row = (uint8 *)Buffer->Memory + (MinX * Buffer->BytesPerPixel) + (MinY * Buffer->Pitch);
    for (int32 Y = MinY; Y < MaxY; Y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int32 X = MinX; X < MaxX; X++)
        {
            *Pixel++ = Color;
        }
        
        Row += Buffer->Pitch;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
             ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *State = (game_state *)Memory->PermanentStorageBytes;
    if (!Memory->IsInitialized)
        Memory->IsInitialized = true;

    int32 Increment = 10;

    for (int32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog)
        {
            // analog movement tuning
            // I don't have a controller to test this
        }
        else
        {
            // digital movement tuning
        }
    }

    DrawRectangle(VideoBuffer, 0.0f, 0.0f, (real32)VideoBuffer->Width, (real32)VideoBuffer->Width, 0xFFFAFAFA);
    DrawRectangle(VideoBuffer, 0.0f, 0.0f, 80.0f, 80.0f, 0xFF88AACC);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *State = (game_state *)Memory->PermanentStorageBytes;
    OutputSineWave(SoundBuffer, 400);
}



#if 0
void RenderWeirdGradient(game_video_buffer *Buffer, int32 BlueOffset, int32 GreenOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;
    for (int Y = 0;Y < Buffer->Height; Y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer->Width; X++)
        {
            // memory Order: BB GG RR XX
            // 0xXXRRGGBB
            uint8 Blue = (uint8)(X + BlueOffset);
            uint8 Green = (uint8)(Y + GreenOffset);
            *Pixel++ = ((Green << 8) | Blue );
        }
        Row += Buffer->Pitch;
    }
}
#endif
