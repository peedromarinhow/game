#ifndef FONTS_H
#define FONTS_H

#include "lingo.h"
#include "platform.h"
#include "renderer.h"

#pragma comment(lib, "freetype")

#include "libs/ft2build.h"
#include FT_FREETYPE_H

internal texture LoadFontFreetype(platform_api *p, FT_Library *FreeTypeLib, c8 *Filename) {
    FT_Face Face = {0};
    if (FT_New_Face(*FreeTypeLib, Filename, 0, &Face))
        ;
    
    i32 Height  = 32;
    u32 NoChars = 128;
    i32 Padding = 2;
    f32 RequiredAreaForAtlas = 0;
    
    FT_Set_Pixel_Sizes(Face, 0, Height);

    for (u32 Char = 0; Char < NoChars; Char++) {
        if (FT_Load_Char(Face, Char + 32, FT_LOAD_RENDER))
            continue;
        
        RequiredAreaForAtlas += (Face->glyph->bitmap.width + 2 * Padding) *
                                (Face->glyph->bitmap.rows  + 2 * Padding);
    }

    f32 GuessSize = Sqrt(RequiredAreaForAtlas) * 1.3f;
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

        OffsetX += (Face->glyph->bitmap.width + 2 * Padding);

        if (OffsetX >= (i32)(AtlasImage.w - Face->glyph->bitmap.width - Padding)) {
            OffsetX = Padding;
            OffsetY += (Height + 2 * Padding);
            if (OffsetY > (AtlasImage.h - Height - Padding))
                break;
        }
    }

    texture Atlas;
    Atlas.w = AtlasImage.w;
    Atlas.h = AtlasImage.h;

    glGenTextures(1, &Atlas.Id);
    glBindTexture(GL_TEXTURE_2D, Atlas.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Atlas.w, Atlas.h,
                 0, GL_RED, GL_UNSIGNED_BYTE, AtlasImage.Data);
    
    FT_Done_Face(Face);
    
    return Atlas;
}

#endif//FONTS_H