#if 0

void OutputSineWave(app_sound_buffer *SoundBuffer, i32 ToneFrequency) {
    i16 ToneVolume = 3000;
    i32 WavePeriod = SoundBuffer->SamplesPerSecond / ToneFrequency;

    i16* SampleOut = SoundBuffer->Samples;
    for (i32 SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
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
            AppState->SineT -= 2.0f * PI32;
#endif
    }
}

void RenderFire(app_input *Input, app_video_buffer *VideoBuffer) {
    r32 TileWidth  = 5.0f;
    r32 TileHeight = 5.0f;
    r32 UpperLeftX = 0.0f;
    r32 UpperLeftY = 0.0f;
#define FIRE_TILEMAP_WIDTH 193
#define FIRE_TILEMAP_HEIGHT 108
    // NOT SUPPOSED TO HAVE STATICS, EVER!
    static i32 TileMap[FIRE_TILEMAP_HEIGHT][FIRE_TILEMAP_WIDTH] = {};
    for (i32 X = 0; X < FIRE_TILEMAP_WIDTH; X++) {
        TileMap[FIRE_TILEMAP_HEIGHT - 1][X] = 32;
    }

    DrawRectangle(VideoBuffer, 0.0f, 0.0f, (r32)VideoBuffer->Width, (r32)VideoBuffer->Width, 0.0f, 0.0f, 0.0f);

    for (i32 Row = 0; Row < FIRE_TILEMAP_HEIGHT; Row++) {
        for (i32 Column = 0; Column < FIRE_TILEMAP_WIDTH; Column++) {
            i32 Tile = TileMap[Row][Column];
            i32 Decay = 0;

            if (Row + 1 < FIRE_TILEMAP_HEIGHT) {
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

typedef struct _tile_map {
    i32 CountX;
    i32 CountY;

    r32 UpperLeftX;
    r32 UpperLeftY;
    r32 TileWidth;
    r32 TileHeight;

    u32 *Tiles;
} tile_map;

internal b32 IsTileMapPointEmpty(tile_map *TileMap, r32 TestX, r32 TestY) {
    b32 Empty = false;
    i32 PlayerTileX = TruncateR32ToI32((TestX - TileMap->UpperLeftX) / TileMap->TileWidth);
    i32 PlayerTileY = TruncateR32ToI32((TestY - TileMap->UpperLeftY) / TileMap->TileHeight);
    if ((PlayerTileX >= 0) && (PlayerTileX < TileMap->CountX) &&
        (PlayerTileY >= 0) && (PlayerTileY < TileMap->CountY)) {
        u32 TileMapValue = TileMap->Tiles[PlayerTileY * TileMap->CountX + PlayerTileX];
        Empty = (TileMapValue == 0);
    }

    return Empty;
}

typedef struct _font {
    stbtt_fontinfo Info;
    debug_read_file_result File;
} font;

// note:
//    it looks kinda bad, is this how stb_truetype is supposed to be?
internal void RenderGlyph(thread_context *Thread, app_video_buffer *Buffer,
                          font *Font, char Codepoint, r32 GlyphScale, u32 Color) {
    stbtt_InitFont(&Font->Info, (u8 *)Font->File.Contents, 0);

    i32 GlyphWidth;
    i32 GlyphHeight;
    i32 XOffset;
    i32 YOffset;
    u8 *MonoBitmap = (u8 *)stbtt_GetCodepointBitmap(&Font->Info, 0.0f, stbtt_ScaleForPixelHeight(&Font->Info, GlyphScale),
                                                     Codepoint, &GlyphWidth, &GlyphHeight, &XOffset, &YOffset);
    u8 *DestRow = (u8 *)Buffer->Memory;
    for(int Y = 0; Y < GlyphHeight; Y++) {
        u32 *Dest = (u32 *)DestRow;
        for(int X = 0; X < GlyphWidth; X++) {
            Color = MonoBitmap[Y*GlyphWidth+X];
            // todo:
            //    figure how to color this
            *Dest++ |= Color | (Color << 8) | (Color << 16);
        }
        
        DestRow += Buffer->Pitch;
    }
    
    stbtt_FreeBitmap(MonoBitmap, 0);
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
        for (i32 Y = 0; Y < Header->Height; ++Y) {
            for (i32 X = 0; X < Header->Width; ++X) {
                *SourceDest = (*SourceDest >> 8) | (*SourceDest << 24);
                ++SourceDest;
            }
        }
    }

    return Result;
}

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

internal void Win32DrawVertical (
    win32_offscreen_buffer *BackBuffer,
    i32  X,
    i32  Top,
    i32  Bottom,
    u32 Color
)
{
    if (Top <= 0)
    {
        Top = 0;
    }
    if (Bottom > BackBuffer->Height)
    {
        Bottom = BackBuffer->Height;
    }

    if ((X >= 0) && (X < BackBuffer->Width))
    {
        u8 *Pixel = (u8 *)BackBuffer->Memory + X*BackBuffer->BytesPerPixel + Top*BackBuffer->Pitch;
        for (i32 Y = Top; Y < Bottom; Y++)
        {
            *(u32 *)Pixel = Color;
            Pixel += BackBuffer->Pitch;
        }
    }
}

inline void Win32DrawSoundBufferMarker (
    win32_offscreen_buffer *BackBuffer,
    win32_sound_output     *SoundOutput,
    r32 C,
    i32  PadX,
    i32  Top,
    i32  Bottom,
    DWORD  Value,
    u32 Color
)
{
    r32 Real32X = (C * (r32)Value);
    i32 X = PadX + (i32)Real32X;
    Win32DrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void DEBUGWin32SyncDisplay (
    win32_offscreen_buffer  *BackBuffer,
    i32                    MarkerCount,
    debug_win32_time_marker *Markers,
    i32                    CurrentMarkerIndex,
    win32_sound_output      *SoundOutput,
    r32                   TargetSecondsPerFrame
)
{
    i32 PadX = 16;
    i32 PadY = 16;

    i32 LineHeight = 64;

    r32 C = (r32)(BackBuffer->Width - 2 * PadX) / (r32)SoundOutput->SecondaryBufferSize;
    for (i32 MarkerIndex = 0; MarkerIndex < MarkerCount; MarkerIndex++)
    {
        debug_win32_time_marker *ThisMarker = &Markers[MarkerIndex];
        Assert(ThisMarker->OutputPlayCursor  < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation    < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount   < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor    < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor   < SoundOutput->SecondaryBufferSize);

        DWORD PlayColor = 0x0000FF00;
        DWORD WriteColor = 0xFFFFFFFF;
        DWORD ExpectedFlipColor = 0x000000FF;
        DWORD PlayWindowColor = 0xFFFF00FF;

        i32 Top = PadY;
        i32 Bottom = Top + LineHeight;
        if (MarkerIndex == CurrentMarkerIndex)
        {
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;
            i32 FirstTop = Top;

            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor,  PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);

            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation,  PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);

            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
        }

        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor,  PlayColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + (480 * SoundOutput->BytesPerSample),  PlayColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
    }
}

#endif

