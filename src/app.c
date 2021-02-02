#include "app.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

const u8 FirePalette[][3] = {
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

void OutputSineWave(app_sound_buffer *SoundBuffer, i32 ToneFrequency)
{
    i16 ToneVolume = 3000;
    i32 WavePeriod = SoundBuffer->SamplesPerSecond / ToneFrequency;

    i16* SampleOut = SoundBuffer->Samples;
    for (i32 SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
#if 0
        r32 SineValue = sinf(AppState->SineT);
        i16 SampleValue = (i16)(SineValue * ToneVolume);
#else
        i16 SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
#if 0
        AppState->SineT += 2.0f * PI32 / (r32)WavePeriod;
        if (AppState->SineT > (2.0f * PI32))
        {
            AppState->SineT -= 2.0f * PI32;
        }
#endif
    }
}

internal i32 RoundR32ToI32(r32 Real32)
{
    return (i32)(Real32 + 0.5f);
}

internal u32 RoundR32ToUI32(r32 Real32)
{
    return (u32)(Real32 + 0.5f);
}

internal i32 TruncateR32ToI32(r32 Real32)
{
    return (i32)(Real32);
}

internal u32 TruncateR32ToUI32(r32 Real32)
{
    return (u32)(Real32);
}

void DrawRectangle(app_video_buffer *Buffer,
                   r32 RealMinX, r32 RealMinY,
                   r32 RealMaxX, r32 RealMaxY,
                   r32 R, r32 G, r32 B)
{
    i32 MinX = RoundR32ToI32(RealMinX);
    i32 MinY = RoundR32ToI32(RealMinY);
    i32 MaxX = RoundR32ToI32(RealMaxX);
    i32 MaxY = RoundR32ToI32(RealMaxY);

    u32 Color = (RoundR32ToUI32(R * 255.0f) << 16) |
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

    u8 *Row = (u8 *)Buffer->Memory + (MinX * Buffer->BytesPerPixel) + (MinY * Buffer->Pitch);
    for (i32 Y = MinY; Y < MaxY; Y++)
    {
        u32 *Pixel = (u32 *)Row;
        for (i32 X = MinX; X < MaxX; X++)
        {
            *Pixel++ = Color;
        }

        Row += Buffer->Pitch;
    }
}

typedef struct _font
{
    stbtt_fontinfo Info;
    debug_read_file_result File;
} font;

// note:
//    it looks kinda bad, is this how stb_truetype is supposed to be?
internal void RenderGlyph(thread_context *Thread, app_video_buffer *Buffer,
                          font *Font, char Codepoint, r32 GlyphScale, u32 Color)
{
    stbtt_InitFont(&Font->Info, (u8 *)Font->File.Contents, 0);

    i32 GlyphWidth;
    i32 GlyphHeight;
    u8 *Bitmap = stbtt_GetCodepointBitmap(&Font->Info, 0.0f, stbtt_ScaleForPixelHeight(&Font->Info, 10)*GlyphScale,
                                           Codepoint, &GlyphWidth, &GlyphHeight, 0, 0);
    u8 *DestRow = (u8 *)Buffer->Memory;
    for(int Y = 0; Y < GlyphHeight; Y++)
    {
        u32 *Dest = (u32 *)DestRow;
        for(int X = 0; X < GlyphWidth; X++)
        {
            Color = Bitmap[Y*GlyphWidth+X];
            // todo:
            //    figure how to color this
            *Dest++ |= Color | (Color << 8) | (Color << 16);
        }
        
        DestRow += Buffer->Pitch;
    }
}

void RenderFire(app_input *Input, app_video_buffer *VideoBuffer)
{
    r32 TileWidth  = 5.0f;
    r32 TileHeight = 5.0f;
    r32 UpperLeftX = 0.0f;
    r32 UpperLeftY = 0.0f;
#define FIRE_TILEMAP_WIDTH 193
#define FIRE_TILEMAP_HEIGHT 108
    // NOT SUPPOSED TO HAVE STATICS, EVER!
    static i32 TileMap[FIRE_TILEMAP_HEIGHT][FIRE_TILEMAP_WIDTH] = {};
    for (i32 X = 0; X < FIRE_TILEMAP_WIDTH; X++)
    {
        TileMap[FIRE_TILEMAP_HEIGHT - 1][X] = 32;
    }

    DrawRectangle(VideoBuffer, 0.0f, 0.0f, (r32)VideoBuffer->Width, (r32)VideoBuffer->Width, 0.0f, 0.0f, 0.0f);

    for (i32 Row = 0; Row < FIRE_TILEMAP_HEIGHT; Row++)
    {
        for (i32 Column = 0; Column < FIRE_TILEMAP_WIDTH; Column++)
        {
            i32 Tile = TileMap[Row][Column];
            i32 Decay = 0;

            if (Row + 1 < FIRE_TILEMAP_HEIGHT)
            {
                i32 Below = TileMap[Row + 1][Column];
                Decay = rand() % 2;
                Tile = Below - Decay >= 0? Below - Decay : 0;
            }

            TileMap[Row][Column - Decay] = Tile;

            r32 Red = (r32)FirePalette[Tile][0]/255.0f;
            r32 Green = (r32)FirePalette[Tile][1]/255.0f;
            r32 Blue = (r32)FirePalette[Tile][2]/255.0f;

            r32 MinX = UpperLeftX + ((r32)Column) * TileWidth;
            r32 MinY = UpperLeftX + ((r32)Row) * TileHeight;
            r32 MaxX = MinX + TileWidth;
            r32 MaxY = MinY + TileHeight;

            DrawRectangle(VideoBuffer, MinX, MinY, MaxX, MaxY, Red, Green, Blue);
        }
    }
}


typedef struct _tile_map
{
    i32 CountX;
    i32 CountY;

    r32 UpperLeftX;
    r32 UpperLeftY;
    r32 TileWidth;
    r32 TileHeight;

    u32 *Tiles;
} tile_map;

internal b32 IsTileMapPointEmpty(tile_map *TileMap, r32 TestX, r32 TestY)
{
    b32 Empty = false;
    i32 PlayerTileX = TruncateR32ToI32((TestX - TileMap->UpperLeftX) / TileMap->TileWidth);
    i32 PlayerTileY = TruncateR32ToI32((TestY - TileMap->UpperLeftY) / TileMap->TileHeight);
    if ((PlayerTileX >= 0) && (PlayerTileX < TileMap->CountX) &&
        (PlayerTileY >= 0) && (PlayerTileY < TileMap->CountY))
    {
        u32 TileMapValue = TileMap->Tiles[PlayerTileY * TileMap->CountX + PlayerTileX];
        Empty = (TileMapValue == 0);
    }

    return Empty;
}

#pragma pack(push, 1)
typedef struct _bitmap_header
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    i32 Width;
    i32 Height;
    u16 Planes;
    u16 BitsPerPixel;
} bitmap_header;
#pragma pack(pop)

internal loaded_bitmap DEBUGLoadBMP(debug_platform_read_entire_file ReadFile, thread_context *Thread, char *FileName) {
    loaded_bitmap Result = {};
    debug_read_file_result ReadResult = ReadFile(Thread, FileName);
    if(ReadResult.Contents) {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        u32 *Pixels = (u32 *)((u8 *)ReadResult.Contents + Header->BitmapOffset);
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.Pixels = Pixels;
        u32 *SourceDest = Pixels;
        for (i32 Y = 0; Y < Header->Height; ++Y)
        {
            for (i32 X = 0; X < Header->Width; ++X)
            {
                *SourceDest = (*SourceDest >> 8) | (*SourceDest << 24);
                ++SourceDest;
            }
        }
    }

    return Result;
}

extern "C" APP_UPDATE_AND_RENDER(AppUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
             ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(app_state) <= Memory->PermanentStorageSize);

#define TILEMAP_COUNT_X 16
#define TILEMAP_COUNT_Y 9
    u32 Tiles[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
        {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
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

    TileMap.Tiles = (u32 *)Tiles;

    r32 PlayerWidth = 0.75f * TileMap.TileWidth;
    r32 PlayerHeight = 0.75f * TileMap.TileHeight;

    app_state *State = (app_state *)Memory->PermanentStorageBytes;
    if (!Memory->IsInitialized)
    {
        Memory->IsInitialized = true;
        // https://opengameart.org/content/forest-graveyard-tileset
        // note:
        //    this bitmap is not working, so downloaded "official" bitmap
        //    from https://github.com/cj1128/handmade-hero, is this allowed?
        State->Backdrop = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "test_background.bmp");
        State->PlayerX = 80.0f;
        State->PlayerY = 80.0f;
        State->FontScale = 3.0f;
    }

    i32 Increment = 10;

    for (i32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
    {
        app_controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog)
        {
            // analog movement tuning
            // I don't have a controller to test this
        }
        else
        {
            if (Controller->ActionUp.EndedDown)
                State->FontScale += 0.1f;
            if (Controller->ActionDown.EndedDown)
                State->FontScale -= 0.1f;

            r32 dPlayerX = 0.0f;
            r32 dPlayerY = 0.0f;
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
            r32 NewPlayerX = State->PlayerX + Input->dtForFrame * dPlayerX;
            r32 NewPlayerY = State->PlayerY + Input->dtForFrame * dPlayerY;

            if (IsTileMapPointEmpty(&TileMap, NewPlayerX - 0.5f * PlayerWidth, NewPlayerY) &&
                IsTileMapPointEmpty(&TileMap, NewPlayerX + 0.5f * PlayerWidth, NewPlayerY) &&
                IsTileMapPointEmpty(&TileMap, NewPlayerX, NewPlayerY))
            {
                State->PlayerX = NewPlayerX;
                State->PlayerY = NewPlayerY;
            }
        }
    }

    DrawRectangle(VideoBuffer, 0.0f, 0.0f, (r32)VideoBuffer->Width, (r32)VideoBuffer->Height, 0.0f, 0.0f, 0.0f);
    
#if 1
    RenderFire(Input, VideoBuffer);
    
    loaded_bitmap Backdrop = State->Backdrop;
    i32 BlitWidth = Backdrop.Width;
    i32 BlitHeight = Backdrop.Height;
    if (BlitWidth > VideoBuffer->Width)
    {
        BlitWidth = VideoBuffer->Width;
    }
    if (BlitHeight > VideoBuffer->Height)
    {
        BlitHeight = VideoBuffer->Height;
    }

    u32 *SourceRow = Backdrop.Pixels + Backdrop.Width * (Backdrop.Height - 1);
    u8 *DestRow = (u8 *)VideoBuffer->Memory;
    for(int Y = 0; Y < BlitHeight; Y++)
    {
        u32 *Dest = (u32 *)DestRow;
        u32 *Source = SourceRow;
        for(int X = 0; X < BlitWidth; X++)
        {
            *Dest++ = *Source++;
        }
        
        DestRow += VideoBuffer->Pitch;
        SourceRow -= Backdrop.Width;
    }

    for (i32 Row = 0; Row < TileMap.CountY; Row++)
    {
        for (i32 Column = 0; Column < TileMap.CountX; Column++)
        {
            u32 Tile = Tiles[Row][Column];
            r32 Grey = Tile == 1? 1.0f : 0.5f;
            r32 MinX = TileMap.UpperLeftX + ((r32)Column) * TileMap.TileWidth;
            r32 MinY = TileMap.UpperLeftX + ((r32)Row) *    TileMap.TileHeight;
            r32 MaxX = MinX + TileMap.TileWidth;
            r32 MaxY = MinY + TileMap.TileHeight;

            r32 PlayerR = 1.0f;
            r32 PlayerG = 0.2f;
            r32 PlayerB = 0.2f;
            r32 PlayerLeft = State->PlayerX - (0.5f * PlayerWidth);
            r32 PlayerTop = State->PlayerY - PlayerHeight;

            DrawRectangle(VideoBuffer, MinX, MinY, MaxX, MaxY, Grey, Grey, Grey);
            DrawRectangle(VideoBuffer,
                          PlayerLeft, PlayerTop,
                          PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight,
                          PlayerR, PlayerG, PlayerB);
        }
    }

    DrawRectangle(VideoBuffer, 10.0f, 10.0f, 20.0f, 20.0f, 0.5f, 0.75f, 1.0f);
#endif
    
    font JetBrainsMono = {};
    JetBrainsMono.File = Memory->DEBUGPlatformReadEntireFile(Thread, "d:/fontes/JetBrainsMonoSlashed-2.002/JetBrainsMonoSlashed-Regular.ttf");
    RenderGlyph(Thread, VideoBuffer, &JetBrainsMono, '@', State->FontScale, 0xFFFFFFFF);
}

extern "C" APP_GET_SOUND_SAMPLES(AppGetSoundSamples)
{
    app_state *State = (app_state *)Memory->PermanentStorageBytes;
    OutputSineWave(SoundBuffer, 400);
}



#if 0
void RenderWeirdGradient(app_video_buffer *Buffer, i32 BlueOffset, i32 GreenOffset)
{
    u8 *Row = (u8 *)Buffer->Memory;
    for (int Y = 0;Y < Buffer->Height; Y++)
    {
        u32 *Pixel = (u32 *)Row;
        for (int X = 0; X < Buffer->Width; X++)
        {
            // memory Order: BB GG RR XX
            // 0xXXRRGGBB
            u8 Blue = (u8)(X + BlueOffset);
            u8 Green = (u8)(Y + GreenOffset);
            *Pixel++ = ((Green << 8) | Blue );
        }
        Row += Buffer->Pitch;
    }
}
#endif
