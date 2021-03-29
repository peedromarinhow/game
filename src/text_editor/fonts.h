#ifndef FONTS_H
#define FONTS_H

#include "ft2build.h"
#include FT_FREETYPE_H

#include "lingo.h"
#include "graphics.h"

typedef struct _glyph {
    u32     Codepoint;
    rv2     Advance;
    image   Bitmap;
} glyph;

typedef struct _font {
    u32      NoChars;
    r32      Size;
    glyph   *Chars;
    rectf32 *Rects;
    texture  Atlas;
} font;

internal glyph GetGlyph(FT_Face Face, u32 Codepoint) {
    u32 Index = FT_Get_Char_Index(Face, Codepoint);
    
    if (FT_Load_Glyph(Face, Index, FT_LOAD_DEFAULT)) {
        //todo: error
    }

    if (FT_Render_Glyph(Face->glyph, FT_RENDER_MODE_NORMAL)) {
        //todo: error
    }

    glyph Glyph = {0}; {
        Glyph.Codepoint   = Codepoint;
        Glyph.Advance.x   = Face->glyph->metrics.horiAdvance;
        Glyph.Advance.y   = Face->glyph->metrics.vertAdvance;
        Glyph.Bitmap.Data = Face->glyph->bitmap.buffer;
        Glyph.Bitmap.w    = Face->glyph->metrics.width;
        Glyph.Bitmap.h    = Face->glyph->metrics.height;
    }

    return Glyph;
}

internal font LoadFont(c8 *Filename, r32 Size) {
    FT_Library FreeTypeLib;
    FT_Face    Face;
    file FontFile = LoadFile(Filename);
    if (!FontFile.Data)
         FontFile = LoadFile("c:/windows/fonts/arial.ttf");
    if (FT_Init_FreeType(&FreeTypeLib)) {
        //todo: error
    }
    if (FT_New_Memory_Face(FreeTypeLib, FontFile.Data, FontFile.Size, 0, &Face)) {
        //todo: error
    }
    FT_Set_Char_Size(Face, 0, Size * 64, 0, 0);

    font Font = {0}; {
        Font.NoChars = Face->num_glyphs;
        Font.Size    = Size;
        Font.Chars   = (glyph   *)AllocateMemory(Font.NoChars * sizeof(glyph));
        Font.Rects   = (rectf32 *)AllocateMemory(Font.NoChars * sizeof(rectf32));
    }

    u32 RequiredW = 0;
    u32 RequiredH = 0;
    i32 Padding   = 2;
    for (u32 i = 0; i < Font.NoChars; i++) {
        Font.Chars[i] = GetGlyph(Face, i);
        RequiredW += Font.Chars[i].Bitmap.w + 2 * Padding;
        RequiredH += Font.Chars[i].Bitmap.h + 2 * Padding;
    }

    //note: stolen from raylib
    image Atlas = {0}; {
        Atlas.w    = RequiredW;
        Atlas.h    = RequiredH;
        Atlas.Data = AllocateMemory(Atlas.w * Atlas.h * sizeof(u32));
    }

    i32 OffsetX = Padding;
    i32 OffsetY = Padding;

    for (u32 i = 0; i < Font.NoChars; i++) {
        // Copy pixel data from fc.data to atlas
        for (i32 y = 0; y < Font.Chars[i].Bitmap.h; y++) {
            for (i32 x = 0; x < Font.Chars[i].Bitmap.w; x++) {
                ((u32 *)Atlas.Data)[(OffsetY + y)*Atlas.w + (OffsetX + x)] =
                    ((((u8 *)Font.Chars[i].Bitmap.Data)[y*Font.Chars[i].Bitmap.w + x]) << 24) |
                    ((((u8 *)Font.Chars[i].Bitmap.Data)[y*Font.Chars[i].Bitmap.w + x]) << 16) |
                    ((((u8 *)Font.Chars[i].Bitmap.Data)[y*Font.Chars[i].Bitmap.w + x]) <<  8) |
                    ((((u8 *)Font.Chars[i].Bitmap.Data)[y*Font.Chars[i].Bitmap.w + x]) <<  0);
            }
        }

        Font.Rects[i].x = (f32)OffsetX;
        Font.Rects[i].y = (f32)OffsetY;
        Font.Rects[i].w = (f32)Font.Chars[i].Bitmap.w;
        Font.Rects[i].h = (f32)Font.Chars[i].Bitmap.h;

        OffsetX += (Font.Chars[i].Bitmap.w + 2 * Padding);

        if (OffsetX >= (Atlas.w - Font.Chars[i].Bitmap.w - Padding)) {
            OffsetX  = Padding;
            OffsetY += (Size + 2 * Padding);

            if (OffsetY > (Atlas.h - Size - Padding))
                break;
        }
        
        FreeMemory(Font.Chars[i].Bitmap.Data);
    }

    Font.Atlas.w   = Atlas.w;
    Font.Atlas.h   = Atlas.h;
    Font.Atlas.Id  = 0;

    glGenTextures(1, &Font.Atlas.Id);
    glBindTexture(GL_TEXTURE_2D, Font.Atlas.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Font.Atlas.w, Font.Atlas.h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, Atlas.Data);
    
    FreeMemory(Atlas.Data);

    return Font;
}

#if 0

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
         FontFile = LoadFile("c:/windows/fonts/arial.ttf");
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
#endif

#endif//FONTS_H