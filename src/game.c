#include "game.h"

void OutputSineWave(game_sound_buffer *SoundBuffer, game_state *GameState)
{
    int16 ToneVolume = 3000;
    int32 WavePeriod = SoundBuffer->SamplesPerSecond / GameState->ToneFrequency;

    int16* SampleOut = SoundBuffer->Samples;
    for (int32 SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
#if 1
        real32 SineValue = sinf(GameState->SineT);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        GameState->SineT += 2.0f * PI32 / (real32)WavePeriod;
        if (GameState->SineT > (2.0f * PI32))
        {
            GameState->SineT -= 2.0f * PI32;
        }
    }
}

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
            *Pixel++ = ((Green << 16) | Blue );
        }
        Row += Buffer->Pitch;
    }
}

void RenderPlayer(game_video_buffer *Buffer, int32 PlayerX, int32 PlayerY)
{
    uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch * Buffer->Height;
    int32 PlayerWidth = 50;
    int32 PlayerHeight = 50;
    uint32 Color = 0xFFFFFFFF;
    int32 Top = PlayerY;
    int32 Bottom = Top + PlayerHeight;
    for (int32 X = PlayerX; X < PlayerX + PlayerWidth; X++)
    {
        uint8 *Pixel = (uint8 *)Buffer->Memory + X*Buffer->BytesPerPixel + Top*Buffer->Pitch;
        for (int32 Y = Top; Y < Bottom; Y++)
        {
            if ((Pixel >= Buffer->Memory) && ((Pixel + 4) <= EndOfBuffer))
            {
                *(uint32 *)Pixel = Color;
            }
            Pixel += Buffer->Pitch;
        }
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *State = (game_state *)Memory->PermanentStorageBytes;
    if (!Memory->IsInitialized)
    {
        char *Filename = __FILE__;
        DEBUG_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename);
        if (File.Contents)
        {
            Memory->DEBUGPlatformWriteEntireFile("D:\\code\\game\\data\\test.out", File.ContentsSize, File.Contents);
            Memory->DEBUGPlatformFreeEntireFile(File.Contents);
        }

        State->ToneFrequency = 261;
        State->SineT = 0.0f;

        State->PlayerX = 100;
        State->PlayerY = 100;

        Memory->IsInitialized = true;
    }

    int32 Increment = 10;

    for (int32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog)
        {
            // analog movement tuning
            // I don't have a controller to test this
            State->ToneFrequency = 256 + (int32)(128.0f * Controller->StickAverageX);
            State->BlueOffset += (int32)(4.0f * Controller->StickAverageY);

            State->PlayerX += (int32)(4.0f * Controller->StickAverageX);
            State->PlayerY += (int32)(4.0f * Controller->StickAverageY);
        }
        else
        {
            if (Controller->MoveLeft.EndedDown)
            {
                State->PlayerX -= Increment;
            }
            if (Controller->MoveRight.EndedDown)
            {
                State->PlayerX += Increment;
            }
            if (Controller->MoveUp.EndedDown)
            {
                State->PlayerY += Increment;
            }
            if (Controller->MoveDown.EndedDown)
            {
                State->PlayerY -= Increment;
            }
        }
        if (State->JumpT > 0)
        {
            State->PlayerY -= (int)(10.0f*sinf(2.0f*PI32*State->JumpT));
        }
        if (Controller->ActionDown.EndedDown)
        {
            State->JumpT = 1.0f;
        }
        State->JumpT -= 0.033f/2.0f;
    }
    RenderWeirdGradient(VideoBuffer, State->GreenOffset, State->BlueOffset);
    RenderPlayer(VideoBuffer, State->PlayerX, State->PlayerY);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *State = (game_state *)Memory->PermanentStorageBytes;
    OutputSineWave(SoundBuffer, State);
}
