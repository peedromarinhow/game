#ifndef FONTS_H
#define FONTS_H

#include "lingo.h"
#include "graphics.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

typedef struct _glyph {
    u32 Codepoint;
    i32 Advance;
    i32 w;
    i32 h;
} glyph;

//note: function that returns two things, probably retard
internal struct _glyph__bitmap {
    glyph Glyph;
    image Bitmap;
} GetGlyphAndBitmap(stbtt_fontinfo *Font, r32 Size, u32 Codepoint) {
    i32 w, h, OffX, OffY, Advance;
    u8 *MonoBitmap = stbtt_GetCodepointBitmap(Font, 0, stbtt_ScaleForPixelHeight(Font, Size),
                                              Codepoint, &w, &h, &OffX, &OffY);

    stbtt_GetCodepointHMetrics(Font, Codepoint, &Advance, NULL);

    glyph Glyph = {0}; {
        Glyph.Codepoint = Codepoint;
        Glyph.Advance   = Advance;
        Glyph.w         = w;
        Glyph.h         = h;
    }

    image Bitmap = {0}; {
        Bitmap.w    = w;
        Bitmap.h    = h;
        Bitmap.Data = AllocateMemory(w * h * sizeof(u32));
    }

    u8 *Source  = MonoBitmap;
    u8 *DestRow = (u8 *)Bitmap.Data;
    for (i32 y = 0; y < h; y++) {
        u32 *Dest = (u32 *)DestRow;
        for (i32 x = 0; x < w; x++) {
            u8 Alpha = *Source++;
            *Dest++  = ((Alpha << 24)|
                        (Alpha << 16)|
                        (Alpha <<  8)|
                        (Alpha <<  0));
        }
        DestRow += Bitmap.w * 4;
    }
    stbtt_FreeBitmap(MonoBitmap, 0);

    struct _glyph__bitmap Result = {Glyph, Bitmap};
    return Result;
}

typedef struct _font {
    u32     NoChars;
    glyph  *Chars;
    rect   *Rects;
    texture Atlas;
} font;

internal font LoadFont(memory_arena *Arena, c8 *Filename, u32 NoChars, r32 Size) {
    file FontFile = LoadFileToArena(Arena, Filename);
    stbtt_fontinfo  Font;
    stbtt_InitFont(&Font, FontFile.Data, stbtt_GetFontOffsetForIndex(FontFile.Data, 0));

    font Result = {0}; {
        Result.NoChars = NoChars    ;
        Result.Chars   = (glyph *)AllocateMemory(NoChars * sizeof(glyph));
        Result.Rects   = (rect  *)AllocateMemory(NoChars * sizeof(rect));
    }

    image *CharBitmaps = (image *)AllocateMemory(NoChars * sizeof(image));

    for (u32 i = 0; i < NoChars; i++) {
        struct _glyph__bitmap _GlyphAndBitmap = GetGlyphAndBitmap(&Font, Size, i + 32); {
            Result.Chars[i] = _GlyphAndBitmap.Glyph;
            CharBitmaps [i] = _GlyphAndBitmap.Bitmap;
        }
    }
    //todo:generate font atlas

    return Result;
}

internal texture MakeNothingsTest(memory_arena *Arena, r32 Size) {
    file FontFile = LoadFileToArena(Arena, "eb_garamond.ttf");

    stbtt_fontinfo  Font;
    stbtt_InitFont(&Font, FontFile.Data, stbtt_GetFontOffsetForIndex(FontFile.Data, 0));

    glyph Glyph  = {0};
    image Bitmap = {0};
    struct _glyph__bitmap _GlyphAndBitmap = GetGlyphAndBitmap(&Font, Size, 928); {
        Glyph  = _GlyphAndBitmap.Glyph;
        Bitmap = _GlyphAndBitmap.Bitmap;
    }
    //note: bad idea?

    texture Result = {0}; {
        Result.w  = Bitmap.w;
        Result.h  = Bitmap.h;
        Result.Id = 0;
    }

    glGenTextures(1, &Result.Id);
    glBindTexture(GL_TEXTURE_2D, Result.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Result.w, Result.h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, Bitmap.Data);

    return Result;
}

#if 0
internal texture *MakeNothingsTest(memory_arena *Arena, r32 Size) {
    file FontFile = LoadFileToArena(Arena, "D:/code/platform-layer/data/roboto_regular.ttf");

    stbtt_fontinfo  Font;
    stbtt_InitFont(&Font, FontFile.Data, stbtt_GetFontOffsetForIndex(FontFile.Data, 0));

    texture *Result = AllocateMemory(('z' - 'a') * sizeof(texture));

    for (u32 i = 0; i < ('z' - 'a'); i++) {
        glyph Glyph = GetGlyphAndBitmap(&Font, Size, i + 'a');

        texture Current = {0}; {
            Current.w  = Glyph.Bitmap.w;
            Current.h  = Glyph.Bitmap.h;
            Current.Id = 0;
        }

        glGenTextures(1, &Current.Id);
        glBindTexture(GL_TEXTURE_2D, Current.Id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Current.w, Current.h, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, Glyph.Bitmap.Data);

        Result[i] = Current;
    }

    return Result;
}

typedef struct _font_char {
    i32  Value;
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
