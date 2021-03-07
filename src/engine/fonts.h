#ifndef FONTS_H
#define FONTS_H

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "lingo.h"
#include "graphics.h"

typedef struct _font_char {
    i32  Value;
    u8 *_Bitmap; //todo: get this out of here
    i32  w;
    i32  h;
    i32  OffX;
    i32  OffY;
    i32  Advance;
} font_char;

typedef struct _font {
    i32        Size;
    texture    Texture;
    u32        NoChars;
    font_char *Chars;
} font;

font LoadFont(c8 *Filename, i32 Size, u32 NoChars) {
    //open the font file and retrieve font info
    file File            = LoadFile(Filename);
    stbtt_fontinfo  Info = {0};
    stbtt_InitFont(&Info, File.Data, 0);

    //if "NoChars" as passed 0 then default to 95
    NoChars = (NoChars > 0)? NoChars : 95;
    
    font Result = {0}; {
        Result.Size    = Size;
        Result.NoChars = NoChars;
        Result.Chars   = AllocateMemory(NoChars * sizeof(font_char));
    }

    //iterate over all characters and retreieve bitmaps
    i32 TotalW  = 0;
    i32 TotalH = 0;
    for (u32 c = 0; c < NoChars; c++) {
        Result.Chars[c].Value  = c + 32;
        Result.Chars[c]._Bitmap =
            stbtt_GetCodepointBitmap(&Info, 0, stbtt_ScaleForPixelHeight(&Info, Size),
                                      Result.Chars[c].Value,
                                     &Result.Chars[c].w,    &Result.Chars[c].h,
                                     &Result.Chars[c].OffX, &Result.Chars[c].OffY);
        stbtt_GetCodepointHMetrics(&Info, Result.Chars[c].Value, &Result.Chars[c].Advance, NULL);

        TotalW += Result.Chars[c].w + 4;//4 pixels of padding
        TotalH += Result.Chars[c].h + 4;//4 pixels of padding
    }

    u8 *Atlas = AllocateMemory(TotalW * TotalH);
    //todo: generate this altas
    Result.Texture.w = TotalW;
    Result.Texture.h = TotalH;
    
    glGenTextures(1, &Result.Texture.Id);
    glBindTexture(GL_TEXTURE_2D, Result.Texture.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Result.Texture.w, Result.Texture.h, 0,
                    GL_BGR_EXT, GL_UNSIGNED_BYTE, (void *)Atlas);

    return Result;
}

#if 0
    //iterate over all characters and retreieve bitmap
    for (u32 c = 0; c < NoChars; c++) {
        Result.Chars[c].Value  = c + 32;
        Bitmap =
            stbtt_GetCodepointBitmap(&Info, 0, stbtt_ScaleForPixelHeight(&Info, Size),
                                      Result.Chars[c].Value,
                                     &Result.Chars[c].w,    &Result.Chars[c].h,
                                     &Result.Chars[c].OffX, &Result.Chars[c].OffY);
        stbtt_GetCodepointHMetrics(&Info, Result.Chars[c].Value, &Result.Chars[c].Advance, NULL);
    }

    
    glBindTexture(GL_TEXTURE_2D, Result.Chars[61].TextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Result.Chars[61].w, Result.Chars[61].h, 0,
                    GL_BGR_EXT, GL_UNSIGNED_BYTE, (void *)Bitmap);
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

                    Result.Chars = (font_char *)
                        AllocateMemory(NoChars * sizeof(font_char));

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
