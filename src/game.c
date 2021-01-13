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

internal uint32 RoundR32toUI32(real32 Real32)
{
    uint32 Result = (uint32)(Real32 + 0.5f);
    return Result;
}

void DrawRectangle(game_video_buffer *Buffer, 
                   real32 RealMinX, real32 RealMinY,
                   real32 RealMaxX, real32 RealMaxY,
                   real32 R, real32 G, real32 B)
{
    int32 MinX = RoundR32toI32(RealMinX);
    int32 MinY = RoundR32toI32(RealMinY);
    int32 MaxX = RoundR32toI32(RealMaxX);
    int32 MaxY = RoundR32toI32(RealMaxY);

    uint32 Color = (RoundR32toUI32(R * 255.0f) << 16) |
                   (RoundR32toUI32(G * 255.0f) << 8)  |
                   (RoundR32toUI32(B * 255.0f) << 0);

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

    DrawRectangle(VideoBuffer, 0.0f, 0.0f, (real32)VideoBuffer->Width, (real32)VideoBuffer->Width, 1.0f, 1.0f, 1.0f);
    
    int32 Width =  16;
    int32 Height = 16;
    real32 UpperLeftX = 10.0f;
    real32 UpperLeftY = 10.0f;
    real32 TileWidth  = 25.0f;
    real32 TileHeight = 25.0f;
    // this is not supposed to happen, ever
    static int32 TileMap[16][16] = {
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0},
        {64, 64, 64, 64, 64, 64, 64, 64,  64, 64, 64, 64,  64, 64, 64, 64},
    };
    for (int32 Row = 0; Row < Height; Row++)
    {
        for (int32 Column = 0; Column < Width; Column++)
        {
            int32 Tile = TileMap[Row][Column];
            int32 Decay = 0;
            if (Row + 1 < Height)
            {
                int32 Below = TileMap[Row + 1][Column];
                Decay = rand() % 16;
                Tile = Below - Decay >= 0? Below - Decay : 0;
            }
            TileMap[Row][Column - Decay >= 0? Column - Decay : Column] = Tile;

            real32 Red = ((real32)Tile - 0.1f)/64.0f;

            real32 MinX = UpperLeftX + ((real32)Column) * TileWidth;
            real32 MinY = UpperLeftX + ((real32)Row) * TileHeight;
            real32 MaxX = MinX + TileWidth;
            real32 MaxY = MinY + TileHeight;

            DrawRectangle(VideoBuffer, MinX, MinY, MaxX, MaxY, Red, 0.0f, 0.0f);
        }
    }
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
