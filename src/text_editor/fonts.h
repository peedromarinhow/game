#ifndef FONTS_H
#define FONTS_H

#include "lingo.h"
#include "graphics.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"

typedef struct _glyph {
    u32   Codepoint;
    i32   Advance;
    i32   OffX;
    i32   OffY;
    image Image;
} glyph;

//note: function that returns two things, probably retard
internal glyph GetGlyph(stbtt_fontinfo *Font, r32 Size, u32 Codepoint) {
    i32 w, h, OffX, OffY, Advance;
    f32 ScaleFactor = stbtt_ScaleForPixelHeight(Font, Size);
    u8 *MonoBitmap = stbtt_GetCodepointBitmap(Font, ScaleFactor, ScaleFactor, Codepoint, &w, &h, &OffX, &OffY);

    stbtt_GetCodepointHMetrics(Font, Codepoint, &Advance, NULL);

    glyph Glyph = {0}; {
        Glyph.Codepoint  = Codepoint;
        Glyph.Advance    = (i32)((f32)Advance * ScaleFactor);
        Glyph.OffX       = OffX;
        Glyph.OffY       = OffY;
        Glyph.Image.w    = w;
        Glyph.Image.h    = h;
        Glyph.Image.Data = MonoBitmap;
    }

    return Glyph;
}

typedef struct _font {
    u32      NoChars;
    f32      Size;
    glyph   *Chars;
    rectf32 *Rects;
    texture  Texture;
} font;

internal font LoadFont(c8 *Filename, u32 NoChars, r32 Size) {
    file FontFile = LoadFile(Filename);
    if (!FontFile.Data)
         FontFile = LoadFile("c:windows/fonts/arial.ttf");
    stbtt_fontinfo  Font;
    stbtt_InitFont(&Font, FontFile.Data, stbtt_GetFontOffsetForIndex(FontFile.Data, 0));
    FreeFile(FontFile);

    NoChars = (NoChars > 0)? NoChars : 95;

    font Result = {0}; {
        Result.NoChars = NoChars;
        Result.Size    = Size;
        Result.Chars   = (glyph   *)AllocateMemory(NoChars * sizeof(glyph));
        Result.Rects   = (rectf32 *)AllocateMemory(NoChars * sizeof(rectf32));
    }

    f32 RequiredAreaForAtlas = 0;
    i32 Padding              = 2;
    for (u32 i = 0; i < NoChars; i++) {
        Result.Chars[i] = GetGlyph(&Font, Result.Size, i + 32);
        RequiredAreaForAtlas += (Result.Chars[i].Image.w + 2 * Padding) *
                                (Result.Chars[i].Image.h + 2 * Padding);
    }

    //note: stolen from raylib
    f32 GuessSize = Sqrt(RequiredAreaForAtlas) * 1.3f;
    i32 ImageSize = (i32)powf(2, ceilf(logf((f32)GuessSize)/logf(2)));
    image Atlas = {0}; {
        Atlas.w    = ImageSize;
        Atlas.h    = ImageSize;
        Atlas.Data = AllocateMemory(Atlas.w * Atlas.h * sizeof(u32));
    }

    /* generate font atlas */ {
        i32 OffsetX = Padding;
        i32 OffsetY = Padding;

        // NOTE: Using simple packaging, one char after another
        for (u32 i = 0; i < NoChars; i++) {
            // Copy pixel data from fc.data to atlas
            for (i32 y = 0; y < Result.Chars[i].Image.h; y++) {
                for (i32 x = 0; x < Result.Chars[i].Image.w; x++) {
                    ((u32 *)Atlas.Data)[(OffsetY + y)*Atlas.w + (OffsetX + x)] =
                        ((((u8 *)Result.Chars[i].Image.Data)[y*Result.Chars[i].Image.w + x]) << 24) |
                        ((((u8 *)Result.Chars[i].Image.Data)[y*Result.Chars[i].Image.w + x]) << 16) |
                        ((((u8 *)Result.Chars[i].Image.Data)[y*Result.Chars[i].Image.w + x]) <<  8) |
                        ((((u8 *)Result.Chars[i].Image.Data)[y*Result.Chars[i].Image.w + x]) <<  0);
                }
            }

            // Fill chars rectangles in atlas info
            Result.Rects[i].x = (f32)OffsetX;
            Result.Rects[i].y = (f32)OffsetY;
            Result.Rects[i].w = (f32)Result.Chars[i].Image.w;
            Result.Rects[i].h = (f32)Result.Chars[i].Image.h;

            // Move atlas position X for next character drawing
            OffsetX += (Result.Chars[i].Image.w + 2 * Padding);

            if (OffsetX >= (Atlas.w - Result.Chars[i].Image.w - Padding)) {
                OffsetX = Padding;

                // NOTE: Be careful on offsetY for SDF fonts, by default SDF
                // use an internal padding of 4 pixels, it means char rectangle
                // height is bigger than fontSize, it could be up to (fontSize + 8)
                OffsetY += (Size + 2 * Padding);

                if (OffsetY > (Atlas.h - Size - Padding)) break;
            }

            if (Result.Chars[i].Codepoint == ' ') {
                Result.Rects[i].w = Result.Chars[i].Advance;
            }
            
            FreeMemory(Result.Chars[i].Image.Data);
        }
    }


    Result.Texture.w   = Atlas.w;
    Result.Texture.h   = Atlas.h;
    Result.Texture.Id  = 0;

    glGenTextures(1, &Result.Texture.Id);
    glBindTexture(GL_TEXTURE_2D, Result.Texture.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Result.Texture.w, Result.Texture.h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, Atlas.Data);
    
    FreeMemory(Atlas.Data);

    return Result;
}

i32 GetGlyphIndex(font Font, u32 Codepoint) {
#define TEXT_CHARACTER_NOTFOUND     '?'
#define UNORDERED_CHARSET
#if defined(UNORDERED_CHARSET)
    i32 Index = TEXT_CHARACTER_NOTFOUND;

    for (u32 i = 0; i < Font.NoChars; i++) {
        if (Font.Chars[i].Codepoint == Codepoint) {
            Index = i;
            break;
        }
    }

    return Index;
#else
    return (Codepoint - 32);
#endif
}

#endif//FONTS_H
