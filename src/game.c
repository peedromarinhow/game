#include "game.h"

const uint8 FirePalette[][3] = {
    {7,   7,   7},
    {31,  7,   7},
    {47,  15,  7},
    {71,  15,  7},
    {87,  23,  7},
    {103, 31,  7},
    {119, 31,  7},
    {143, 39,  7},
    {159, 47,  7},
    {175, 63,  7},
    {191, 71,  7},
    {199, 71,  7},
    {223, 79,  7},
    {223, 87,  7},
    {223, 87,  7},
    {215, 95,  7},
    {215, 95,  7},
    {215, 103, 15},
    {207, 111, 15},
    {207, 119, 15},
    {207, 127, 15},
    {207, 135, 23},
    {199, 135, 23},
    {199, 143, 23},
    {199, 151, 31},
    {191, 159, 31},
    {191, 159, 31},
    {191, 167, 39},
    {191, 167, 39},
    {191, 175, 47},
    {183, 175, 47},
    {183, 183, 47},
    {183, 183, 55},
    {207, 207, 111},
    {223, 223, 159},
    {239, 239, 199},
    {255, 255, 255}
};

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

internal int32 RoundR32ToI32(real32 Real32)
{
    return (int32)(Real32 + 0.5f);
}

internal uint32 RoundR32ToUI32(real32 Real32)
{
    return (uint32)(Real32 + 0.5f);
}

internal int32 TruncateR32ToI32(real32 Real32)
{
    return (int32)(Real32);
}

internal uint32 TruncateR32ToUI32(real32 Real32)
{
    return (uint32)(Real32);
}

void DrawRectangle(game_video_buffer *Buffer, 
                   real32 RealMinX, real32 RealMinY,
                   real32 RealMaxX, real32 RealMaxY,
                   real32 R, real32 G, real32 B)
{
    int32 MinX = RoundR32ToI32(RealMinX);
    int32 MinY = RoundR32ToI32(RealMinY);
    int32 MaxX = RoundR32ToI32(RealMaxX);
    int32 MaxY = RoundR32ToI32(RealMaxY);

    uint32 Color = (RoundR32ToUI32(R * 255.0f) << 16) |
                   (RoundR32ToUI32(G * 255.0f) << 8)  |
                   (RoundR32ToUI32(B * 255.0f) << 0);

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

void RenderFire(game_input *Input, game_video_buffer *VideoBuffer)
{
    real32 TileWidth  = 5.0f;
    real32 TileHeight = 5.0f;
    real32 UpperLeftX = 0.0f;
    real32 UpperLeftY = 0.0f;
#define FIRE_TILEMAP_WIDTH 193
#define FIRE_TILEMAP_HEIGHT 108
    // NOT SUPPOSED TO HAVE STATICS, EVER!
    static int32 TileMap[FIRE_TILEMAP_HEIGHT][FIRE_TILEMAP_WIDTH] = {};
    for (int32 X = 0; X < FIRE_TILEMAP_WIDTH; X++)
    {
        TileMap[FIRE_TILEMAP_HEIGHT - 1][X] = 32;
    }

    DrawRectangle(VideoBuffer, 0.0f, 0.0f, (real32)VideoBuffer->Width, (real32)VideoBuffer->Width, 0.0f, 0.0f, 0.0f);

    for (int32 Row = 0; Row < FIRE_TILEMAP_HEIGHT; Row++)
    {
        for (int32 Column = 0; Column < FIRE_TILEMAP_WIDTH; Column++)
        {
            int32 Tile = TileMap[Row][Column];
            int32 Decay = 0;

            if (Row + 1 < FIRE_TILEMAP_HEIGHT)
            {
                int32 Below = TileMap[Row + 1][Column];
                Decay = rand() % 2;
                Tile = Below - Decay >= 0? Below - Decay : 0;
            }
            
            TileMap[Row][Column - Decay] = Tile;

            real32 Red = (real32)FirePalette[Tile][0]/255.0f;
            real32 Green = (real32)FirePalette[Tile][1]/255.0f;
            real32 Blue = (real32)FirePalette[Tile][2]/255.0f;

            real32 MinX = UpperLeftX + ((real32)Column) * TileWidth;
            real32 MinY = UpperLeftX + ((real32)Row) * TileHeight;
            real32 MaxX = MinX + TileWidth;
            real32 MaxY = MinY + TileHeight;

            DrawRectangle(VideoBuffer, MinX, MinY, MaxX, MaxY, Red, Green, Blue);
        }
    }
}

typedef struct _tile_map
{
    int32 CountX;
    int32 CountY;

    real32 UpperLeftX;
    real32 UpperLeftY;
    real32 TileWidth;
    real32 TileHeight;

    uint32 *Tiles;
} tile_map;

internal bool32 IsTileMapPointEmpty(tile_map *TileMap, real32 TestX, real32 TestY)
{
    bool32 Empty = false;
    int32 PlayerTileX = TruncateR32ToI32((TestX - TileMap->UpperLeftX) / TileMap->TileWidth);
    int32 PlayerTileY = TruncateR32ToI32((TestY - TileMap->UpperLeftY) / TileMap->TileHeight);
    if ((PlayerTileX >= 0) && (PlayerTileX < TileMap->CountX) &&
        (PlayerTileY >= 0) && (PlayerTileY < TileMap->CountY))
    {
        uint32 TileMapValue = TileMap->Tiles[PlayerTileY * TileMap->CountX + PlayerTileX];
        Empty = (TileMapValue == 0);
    }

    return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
             ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
#define TILEMAP_COUNT_X 16
#define TILEMAP_COUNT_Y 9
    uint32 Tiles[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
        {{1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
         {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
         {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
         {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
         {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
         {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
         {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
         {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
         {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1}};
    tile_map TileMap;
    TileMap.CountX = TILEMAP_COUNT_X;
    TileMap.CountY = TILEMAP_COUNT_Y;

    TileMap.UpperLeftX = -15;
    TileMap.UpperLeftY = 0;
    TileMap.TileWidth  = 30;
    TileMap.TileHeight = 30;

    TileMap.Tiles = (uint32 *)Tiles;
    
    real32 PlayerWidth = 0.75f * TileMap.TileWidth;
    real32 PlayerHeight = 0.75f * TileMap.TileHeight;

    game_state *State = (game_state *)Memory->PermanentStorageBytes;
    if (!Memory->IsInitialized)
    {
        Memory->IsInitialized = true;
        State->PlayerX = 80.0f;
        State->PlayerY = 80.0f;
    }

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
            real32 dPlayerX = 0.0f;
            real32 dPlayerY = 0.0f;
            if (Controller->MoveUp.EndedDown)
                dPlayerY = -1.0f;
            if (Controller->MoveDown.EndedDown)
                dPlayerY =  1.0f;
            if (Controller->MoveLeft.EndedDown)
                dPlayerX = -1.0f;
            if (Controller->MoveRight.EndedDown)
                dPlayerX =  1.0f;

            dPlayerX *= 128.0f;
            dPlayerY *= 128.0f;

            // diagonals are faster, fix with vectors
            real32 NewPlayerX = State->PlayerX + Input->dtForFrame * dPlayerX;
            real32 NewPlayerY = State->PlayerY + Input->dtForFrame * dPlayerY;
        
            if (IsTileMapPointEmpty(&TileMap, NewPlayerX - 0.5f * PlayerWidth, NewPlayerY) &&
                IsTileMapPointEmpty(&TileMap, NewPlayerX + 0.5f * PlayerWidth, NewPlayerY) &&
                IsTileMapPointEmpty(&TileMap, NewPlayerX, NewPlayerY))
            {
                State->PlayerX = NewPlayerX;
                State->PlayerY = NewPlayerY;
            }
        }
    }

    DrawRectangle(VideoBuffer, 0.0f, 0.0f, (real32)VideoBuffer->Width, (real32)VideoBuffer->Width, 0.5f, 0.75f, 1.0f);
    RenderFire(Input, VideoBuffer);

    for (int32 Row = 0; Row < TileMap.CountY; Row++)
    {
        for (int32 Column = 0; Column < TileMap.CountX; Column++)
        {
            uint32 Tile = Tiles[Row][Column];
            real32 Grey = Tile == 1? 1.0f : 0.5f;
            real32 MinX = TileMap.UpperLeftX + ((real32)Column) * TileMap.TileWidth;
            real32 MinY = TileMap.UpperLeftX + ((real32)Row) *    TileMap.TileHeight;
            real32 MaxX = MinX + TileMap.TileWidth;
            real32 MaxY = MinY + TileMap.TileHeight;

            real32 PlayerR = 1.0f;
            real32 PlayerG = 0.2f;
            real32 PlayerB = 0.2f;
            real32 PlayerLeft = State->PlayerX - (0.5f * PlayerWidth);
            real32 PlayerTop = State->PlayerY - PlayerHeight;

            DrawRectangle(VideoBuffer, MinX, MinY, MaxX, MaxY, Grey, Grey, Grey);
            DrawRectangle(VideoBuffer,
                          PlayerLeft, PlayerTop,
                          PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight,
                          PlayerR, PlayerG, PlayerB);
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