#ifndef FONTS_H
#define FONTS_H

#include "lingo.h"
#include "platform.h"
// #include "renderer.h"

#pragma comment(lib, "freetype")

#include "libs/ft2build.h"
#include FT_FREETYPE_H

internal font LoadFont(platform_api *p, FT_Library *FreeTypeLib, c8 *Filename, i32 Height) {
    FT_Face Face = {0};
    font    Font = {0};
    if (FT_New_Face(*FreeTypeLib, Filename, 0, &Face))
        return Font;

    u32 NoChars = Face->num_glyphs;
    i32 Padding = 2;
    f32 RequiredAreaForAtlas = 0;
    
    FT_Set_Pixel_Sizes(Face, 0, Height);

    Font.NoChars = NoChars;
    Font.Height    = Height;
    Font.Ascender  = Face->ascender;
    Font.Descender = Face->descender;
    Font.LineGap   = 0;
    Font.Advances = p->AllocateMemory(NoChars * sizeof(rv2));
    Font.Bearings = p->AllocateMemory(NoChars * sizeof(rv2));
    Font.Rects    = p->AllocateMemory(NoChars * sizeof(rect));

    for (u32 Char = 0; Char < NoChars; Char++) {
        if (FT_Load_Char(Face, Char + 32, FT_LOAD_RENDER))
            continue;
        RequiredAreaForAtlas += (Face->glyph->bitmap.width + 2 * Padding) *
                                (Face->glyph->bitmap.rows  + 2 * Padding);
    }

    f32 GuessSize = Sqrt(RequiredAreaForAtlas);
    i32 ImageSize = (i32)powf(2, ceilf(logf((f32)GuessSize)/logf(2)));
    image AtlasImage = {
        .w    = ImageSize,
        .h    = ImageSize,
        .Data = p->AllocateMemory(ImageSize * ImageSize)
    };

    i32 OffsetX = Padding;
    i32 OffsetY = Padding;

    for (u32 Char = 0; Char < NoChars; Char++) {
        if (FT_Load_Char(Face, Char + 32, FT_LOAD_RENDER))
            continue;

        i32 w = Face->glyph->bitmap.width;
        i32 h = Face->glyph->bitmap.rows;

        for (i32 y = 0; y < h; y++) {
            for (i32 x = 0; x < w; x++) {
                ((u8 *)AtlasImage.Data)[(OffsetY + y) * AtlasImage.w + (OffsetX + x)] =
                    (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]);
            }
        }

        Font.Advances[Char] = Face->glyph->advance.x;
        Font.Bearings[Char] = rv2_(Face->glyph->bitmap_left, Face->glyph->bitmap_top);

        Font.Rects[Char] = rect_(OffsetX, OffsetY, w, h);
        OffsetX += (w + 2 * Padding);

        if (OffsetX >= (i32)(AtlasImage.w - w - Padding)) {
            OffsetX = Padding;
            OffsetY += (Height + 2 * Padding);
            if (OffsetY > (AtlasImage.h - Height - Padding))
                break;
        }
    }

    Font.Atlas.w = AtlasImage.w;
    Font.Atlas.h = AtlasImage.h;

    glGenTextures(1, &Font.Atlas.Id);
    glBindTexture(GL_TEXTURE_2D, Font.Atlas.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Font.Atlas.w, Font.Atlas.h,
                 0, GL_RED, GL_UNSIGNED_BYTE, AtlasImage.Data);
    
    FT_Done_Face(Face);
    
    return Font;
}

#endif//FONTS_H