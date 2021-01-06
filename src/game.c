#include "game.h"

void OutputSineWave(game_sound_buffer *SoundBuffer, game_state *GameState)
{
    int16 ToneVolume = 3000;
    int32 WavePeriod = SoundBuffer->SamplesPerSecond / GameState->ToneFrequency;

    int16* SampleOut = SoundBuffer->Samples;
    for (int32 SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        real32 SineValue = sinf(GameState->SineT);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
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
            //uint8 Blue = (uint8)(X + BlueOffset);
            //uint8 Green = (uint8)(Y + GreenOffset);
            *Pixel++ = 0xFF000000;//((Green << 8) | Blue );
        }
        Row += Buffer->Pitch;
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

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *State = (game_state *)Memory->PermanentStorageBytes;
    OutputSineWave(SoundBuffer, State);
}

#if BUILD_WIN32
#include <windows.h>
BOOL WINAPI DllMain (
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpReserved
)
{
    return TRUE;
}
#endif