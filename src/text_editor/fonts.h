#ifndef FONTS_H
#define FONTS_H

#include "lingo.h"
#include "platform.h"
// #include "renderer.h"

#pragma comment(lib, "freetype")

#include "libs/ft2build.h"
#include FT_FREETYPE_H

typedef struct _font_ {
    u32 NoChars;
    i32 Height;
    i32 Ascender;
    i32 Descender;
    i32 LineGap;
    rv2  *Advances;
    rv2  *Bearings;
    rect *Rects;
    texture Atlas;
} font_;

internal font_ LoadFontFreetype(platform_api *p, FT_Library *FreeTypeLib, c8 *Filename) {
    FT_Face Face = {0};
    font_   Font = {0};
    if (FT_New_Face(*FreeTypeLib, Filename, 0, &Face))
        return Font;
    
    i32 Height  = 64;
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

        Font.Advances[Char] = rv2_(Face->glyph->advance.x,   Face->glyph->advance.y);
        Font.Bearings[Char] = rv2_(Face->glyph->bitmap_left, Face->glyph->bitmap_top);

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

        for (u32 y = 0; y < Face->glyph->bitmap.rows; y++) {
            for (u32 x = 0; x < Face->glyph->bitmap.width; x++) {
                ((u8 *)AtlasImage.Data)[(OffsetY + y) * AtlasImage.w + (OffsetX + x)] =
                    (((u8 *)Face->glyph->bitmap.buffer)[y*Face->glyph->bitmap.width + x]);
            }
        }

        Font.Rects[Char] = rect_(OffsetX, OffsetY, Face->glyph->bitmap.width, Face->glyph->bitmap.rows);
        OffsetX += (Face->glyph->bitmap.width + 2 * Padding);

        if (OffsetX >= (i32)(AtlasImage.w - Face->glyph->bitmap.width - Padding)) {
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