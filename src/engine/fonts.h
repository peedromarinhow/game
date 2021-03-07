#ifndef FONTS_H
#define FONTS_H

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "lingo.h"
#include "graphics.h"

typedef struct _font_character {
    i32   Value;
    i32   OffsetX;
    i32   OffsetY;
    i32   Advance;
    image Image;
} font_character;

typedef struct _font {
    i32             Size;
    u32             NoChars;
    texture         Texture;
    rect           *Rects;
    font_character *Chars;
} font;

void LoadFont(c8 *Filename, i32 Size, u32 NoChars) {
    file File            = LoadFile(Filename);
    stbtt_fontinfo  Info = {0};
    stbtt_InitFont(&Info, File.Data, 0);

    i32   w     = 0;
    i32   h     = 0;
    i32   OffX  = 0;
    i32   OffY  = 0;
    
    u8* MonoBitmap = stbtt_GetCodepointBitmap(&Info, 0,
                                               stbtt_ScaleForPixelHeight(&Info, Size),
                                              'N', &w,  &h, &OffX, &OffY);

    image Result = {0}; {
        Result.w    = w;
        Result.h    = h;
        Result.Data = AllocateMemory(Result.w * Result.h * sizeof(u32));
    }

    u8 *Source  = MonoBitmap;
    u8 *DestRow = (u8 *)Result.Data;
    for (i32 y = 0; y < h; y++) {
        u32 *Dest = (u32 *)DestRow;
        for (i32 x = 0; x < w; x++) {
            u8 Alpha = *Source++;
            *Dest++ = ((Alpha << 24) |
                       (Alpha << 16) |
                       (Alpha <<  8) |
                       (Alpha <<  0));
        }
        DestRow += Result.w * 4;
    }

    stbi_write_bmp("im.bmp", Result.w, Result.h, 0, Result.Data);
    stbtt_FreeBitmap(MonoBitmap, 0);
}

#if 0
font LoadFont(c8 *Filename, i32 FontSize, u32 NoChars) {
    font Result = {0}; {
        Result.Size    = FontSize;
        Result.NoChars = NoChars;
        Result.Chars   = NULL; {
            file FontFile = LoadFile(Filename);
            
            if (FontFile.Data != NULL) {
                stbtt_fontinfo FontInfo = {0};

                if (stbtt_InitFont(&FontInfo, FontFile.Data, 0)) {
                    f32 ScaleFactor = stbtt_ScaleForPixelHeight(&FontInfo, (f32)FontSize);

                    i32 Ascender;
                    i32 Descender;
                    i32 LineGap;
                    stbtt_GetFontVMetrics(&FontInfo, &Ascender, &Descender, &LineGap);

                    NoChars = (NoChars > 0)? NoChars : 95;

                    Result.Chars = (font_character *)
                        AllocateMemory(NoChars * sizeof(font_character));

                    for (u32 i = 0; i < NoChars; i++) {
                        i32 ChW = 0;
                        i32 ChH = 0;

                        Result.Chars[i].Value      = i + 32;
                        Result.Chars[i].Image.Data =
                            stbtt_GetCodepointBitmap(&FontInfo, ScaleFactor, ScaleFactor,
                                                      Result.Chars[i].Value,
                                                     &ChW, &ChH,
                                                     &Result.Chars[i].OffsetX,
                                                     &Result.Chars[i].OffsetY);
                        
                        stbtt_GetCodepointHMetrics(&FontInfo, Result.Chars[i].Value,
                                                   &Result.Chars[i].Advance, NULL);

                        Result.Chars[i].Advance = (i32)
                            ((f32)Result.Chars[i].Advance * ScaleFactor);

                        Result.Chars[i].Image.w      = ChW;
                        Result.Chars[i].Image.h      = ChH;
                        Result.Chars[i].Image.Format = 1;

                        Result.Chars[i].OffsetX += (i32)((f32)Ascender * ScaleFactor);

                        if (Result.Chars[i].Value == 32) {
                            image BlankImage = {0}; {
                                BlankImage.Data   = AllocateMemory(Result.Chars[i].Advance * FontSize * sizeof(color4f));
                                BlankImage.w      = Result.Chars[i].Advance;
                                BlankImage.h      = FontSize;
                                BlankImage.Format = 1; //uncompressed grayscale
                            }
                            Result.Chars[i].Image = BlankImage;
                        }
                    }
                }
                else {
                    ReportError("ERROR", "Could not process ttf file");
                }
                FreeFile(FontFile);
            }
            else {
                ReportError("ERROR", "Could not load font file");
            }
        }
    }

    return Result;
}
#endif

#endif//FONTS_H
