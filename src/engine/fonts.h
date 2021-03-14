#ifndef FONTS_H
#define FONTS_H

#include "lingo.h"
#include "graphics.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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
        image MonoAtlas = {0}; {
            MonoAtlas.w    = ImageSize;
            MonoAtlas.h    = ImageSize;
            MonoAtlas.Data = AllocateMemory(Atlas.w * Atlas.h);
        }

        i32 OffsetX = Padding;
        i32 OffsetY = Padding;

        // NOTE: Using simple packaging, one char after another
        for (u32 i = 0; i < NoChars; i++) {
            // Copy pixel data from fc.data to atlas
            for (i32 y = 0; y < Result.Chars[i].Image.h; y++) {
                for (i32 x = 0; x < Result.Chars[i].Image.w; x++) {
                    ((unsigned char *)MonoAtlas.Data)[(OffsetY + y)*MonoAtlas.w + (OffsetX + x)] =
                        ((unsigned char *)Result.Chars[i].Image.Data)[y*Result.Chars[i].Image.w + x];
                }
            }

            // Fill chars rectangles in atlas info
            Result.Rects[i].x = (f32)OffsetX;
            Result.Rects[i].y = (f32)OffsetY;
            Result.Rects[i].w = (f32)Result.Chars[i].Image.w;
            Result.Rects[i].h = (f32)Result.Chars[i].Image.h;

            // Move atlas position X for next character drawing
            OffsetX += (Result.Chars[i].Image.w + 2 * Padding);

            if (OffsetX >= (MonoAtlas.w - Result.Chars[i].Image.w - Padding)) {
                OffsetX = Padding;

                // NOTE: Be careful on offsetY for SDF fonts, by default SDF
                // use an internal padding of 4 pixels, it means char rectangle
                // height is bigger than fontSize, it could be up to (fontSize + 8)
                OffsetY += (Size + 2 * Padding);

                if (OffsetY > (MonoAtlas.h - Size - Padding)) break;
            }
        }

        u8 *Source  = MonoAtlas.Data;
        u8 *DestRow = (u8 *)Atlas.Data;
        for (i32 y = 0; y < MonoAtlas.h; y++) {
            u32 *Dest = (u32 *)DestRow;
            for (i32 x = 0; x < MonoAtlas.w; x++) {
                u8 Alpha = *Source++;
                *Dest++  = ((Alpha << 24)|
                            (Alpha << 16)|
                            (Alpha <<  8)|
                            (Alpha <<  0));
            }
            DestRow += Atlas.w * 4;
        }
        FreeMemory(MonoAtlas.Data);
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
#define TEXT_CHARACTER_NOTFOUND     63      // Character: '?'
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

void gDrawText(font Font, u32 *Text, rv2 Pos, f32 Size, f32 Spacing, color4f Tint) {
    i32 TextLenght      = ArrayCount(Text);
    f32 ScaleFactor     = Size/Font.Size;
    f32 CharacterOffset = 0;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS); {
        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
        i32 i = 0;
        while (Text[i] != '\0') {
            i32 Index     = GetGlyphIndex(Font, Text[i]);
            i32 Codepoint = Font.Chars[Index].Codepoint;
            glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);

            rectf32 Rect = Font.Rects[Index];
            f32 w        = Font.Texture.w;
            f32 h        = Font.Texture.h;

            glTexCoord2f(Rect.x/w, Rect.y/h);
            glVertex2f  (Pos.x + CharacterOffset, Pos.y);

            glTexCoord2f((Rect.x + Rect.w)/w, Rect.y/h);
            glVertex2f  (Pos.x + CharacterOffset + Rect.w, Pos.y);

            glTexCoord2f((Rect.x + Rect.w)/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharacterOffset + Rect.w, Pos.y + Rect.h);

            glTexCoord2f(Rect.x/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharacterOffset, Pos.y + Rect.h);

            CharacterOffset += Font.Chars[Index].Advance * ScaleFactor;
            i++;
        }
    } glEnd();
        
}

/* the charset of EB Garamond
"A︎B︎C︎Č︎Ć︎D︎Đ︎E︎F︎G︎H︎I︎J︎K︎L︎M︎N︎O︎P︎Q︎R︎S︎Š︎T︎U︎V︎W︎X︎Y︎Z︎Ž︎a︎b︎c︎č︎ć︎d︎đ︎e︎f︎g︎h︎i︎j︎k︎l︎m︎n︎o︎p︎"
"q︎r︎s︎š︎t︎u︎v︎w︎x︎y︎z︎ž︎А︎Б︎В︎Г︎Ґ︎Д︎Ђ︎Е︎Ё︎Є︎Ж︎З︎Ѕ︎И︎І︎Ї︎Й︎Ј︎К︎Л︎Љ︎М︎Н︎Њ︎О︎П︎Р︎С︎Т︎Ћ︎У︎Ў︎Ф︎Х︎Ц︎Ч︎Џ︎Ш︎"
"Щ︎Ъ︎Ы︎Ь︎Э︎Ю︎Я︎а︎б︎в︎г︎ґ︎д︎ђ︎е︎ё︎є︎ж︎з︎ѕ︎и︎і︎ї︎й︎ј︎к︎л︎љ︎м︎н︎њ︎о︎п︎р︎с︎т︎ћ︎у︎ў︎ф︎х︎ц︎ч︎џ︎ш︎щ︎ъ︎ы︎ь︎э︎"
"ю︎я︎Α︎Β︎Γ︎Δ︎Ε︎Ζ︎Η︎Θ︎Ι︎Κ︎Λ︎Μ︎Ν︎Ξ︎Ο︎Π︎Ρ︎Σ︎Τ︎Υ︎Φ︎Χ︎Ψ︎Ω︎α︎β︎γ︎δ︎ε︎ζ︎η︎θ︎ι︎κ︎λ︎μ︎ν︎ξ︎ο︎π︎ρ︎σ︎τ︎υ︎φ︎χ︎ψ︎ω︎ά︎Ά︎"
"έ︎Έ︎Ή︎ί︎ϊ︎ΐ︎Ί︎ό︎Ό︎ύ︎ΰ︎ϋ︎Ύ︎Ϋ︎ὰ︎ά︎ὲ︎έ︎ὴ︎ή︎ὶ︎ί︎ὸ︎ό︎ὺ︎ύ︎ὼ︎ώ︎Ώ︎Ă︎Â︎Ê︎Ô︎Ơ︎Ư︎ă︎â︎ê︎ô︎ơ︎ư︎1︎2︎3︎4︎5︎6︎7︎8︎9︎0︎‘︎?︎’︎“︎"
"!︎”︎(︎%︎)︎[︎#︎]︎{︎@︎}︎/︎&︎\︎<︎-︎+︎÷︎×︎=︎>︎®︎©︎$︎€︎£︎¥︎¢︎:︎;︎,︎.︎*︎"
    0x0041, 0xFE0E, 0x0042, 0xFE0E, 0x0043, 0xFE0E, 0x010C, 0xFE0E, 0x0106, 0xFE0E, 0x0044,
    0xFE0E, 0x0110, 0xFE0E, 0x0045, 0xFE0E, 0x0046, 0xFE0E, 0x0047, 0xFE0E, 0x0048, 0xFE0E,
    0x0049, 0xFE0E, 0x004A, 0xFE0E, 0x004B, 0xFE0E, 0x004C, 0xFE0E, 0x004D, 0xFE0E, 0x004E,
    0xFE0E, 0x004F, 0xFE0E, 0x0050, 0xFE0E, 0x0051, 0xFE0E, 0x0052, 0xFE0E, 0x0053, 0xFE0E,
    0x0160, 0xFE0E, 0x0054, 0xFE0E, 0x0055, 0xFE0E, 0x0056, 0xFE0E, 0x0057, 0xFE0E, 0x0058,
    0xFE0E, 0x0059, 0xFE0E, 0x005A, 0xFE0E, 0x017D, 0xFE0E, 0x0061, 0xFE0E, 0x0062, 0xFE0E, 0x0063,
    0xFE0E, 0x010D, 0xFE0E, 0x0107, 0xFE0E, 0x0064, 0xFE0E, 0x0111, 0xFE0E, 0x0065, 0xFE0E, 0x0066,
    0xFE0E, 0x0067, 0xFE0E, 0x0068, 0xFE0E, 0x0069, 0xFE0E, 0x006A, 0xFE0E, 0x006B, 0xFE0E, 0x006C,
    0xFE0E, 0x006D, 0xFE0E, 0x006E, 0xFE0E, 0x006F, 0xFE0E, 0x0070, 0xFE0E, 0x0071, 0xFE0E, 0x0072,
    0xFE0E, 0x0073, 0xFE0E, 0x0161, 0xFE0E, 0x0074, 0xFE0E, 0x0075, 0xFE0E, 0x0076, 0xFE0E, 0x0077,
    0xFE0E, 0x0078, 0xFE0E, 0x0079, 0xFE0E, 0x007A, 0xFE0E, 0x017E, 0xFE0E, 0x0410, 0xFE0E, 0x0411,
    0xFE0E, 0x0412, 0xFE0E, 0x0413, 0xFE0E, 0x0490, 0xFE0E, 0x0414, 0xFE0E, 0x0402, 0xFE0E, 0x0415,
    0xFE0E, 0x0401, 0xFE0E, 0x0404, 0xFE0E, 0x0416, 0xFE0E, 0x0417, 0xFE0E, 0x0405, 0xFE0E, 0x0418,
    0xFE0E, 0x0406, 0xFE0E, 0x0407, 0xFE0E, 0x0419, 0xFE0E, 0x0408, 0xFE0E, 0x041A, 0xFE0E, 0x041B,
    0xFE0E, 0x0409, 0xFE0E, 0x041C, 0xFE0E, 0x041D, 0xFE0E, 0x040A, 0xFE0E, 0x041E, 0xFE0E, 0x041F,
    0xFE0E, 0x0420, 0xFE0E, 0x0421, 0xFE0E, 0x0422, 0xFE0E, 0x040B, 0xFE0E, 0x0423, 0xFE0E, 0x040E,
    0xFE0E, 0x0424, 0xFE0E, 0x0425, 0xFE0E, 0x0426, 0xFE0E, 0x0427, 0xFE0E, 0x040F, 0xFE0E, 0x0428,
    0xFE0E, 0x0429, 0xFE0E, 0x042A, 0xFE0E, 0x042B, 0xFE0E, 0x042C, 0xFE0E, 0x042D, 0xFE0E, 0x042E,
    0xFE0E, 0x042F, 0xFE0E, 0x0430, 0xFE0E, 0x0431, 0xFE0E, 0x0432, 0xFE0E, 0x0433, 0xFE0E, 0x0491,
    0xFE0E, 0x0434, 0xFE0E, 0x0452, 0xFE0E, 0x0435, 0xFE0E, 0x0451, 0xFE0E, 0x0454, 0xFE0E, 0x0436,
    0xFE0E, 0x0437, 0xFE0E, 0x0455, 0xFE0E, 0x0438, 0xFE0E, 0x0456, 0xFE0E, 0x0457, 0xFE0E, 0x0439,
    0xFE0E, 0x0458, 0xFE0E, 0x043A, 0xFE0E, 0x043B, 0xFE0E, 0x0459, 0xFE0E, 0x043C, 0xFE0E, 0x043D,
    0xFE0E, 0x045A, 0xFE0E, 0x043E, 0xFE0E, 0x043F, 0xFE0E, 0x0440, 0xFE0E, 0x0441, 0xFE0E, 0x0442,
    0xFE0E, 0x045B, 0xFE0E, 0x0443, 0xFE0E, 0x045E, 0xFE0E, 0x0444, 0xFE0E, 0x0445, 0xFE0E, 0x0446,
    0xFE0E, 0x0447, 0xFE0E, 0x045F, 0xFE0E, 0x0448, 0xFE0E, 0x0449, 0xFE0E, 0x044A, 0xFE0E, 0x044B,
    0xFE0E, 0x044C, 0xFE0E, 0x044D, 0xFE0E, 0x044E, 0xFE0E, 0x044F, 0xFE0E, 0x0391, 0xFE0E, 0x0392,
    0xFE0E, 0x0393, 0xFE0E, 0x0394, 0xFE0E, 0x0395, 0xFE0E, 0x0396, 0xFE0E, 0x0397, 0xFE0E, 0x0398,
    0xFE0E, 0x0399, 0xFE0E, 0x039A, 0xFE0E, 0x039B, 0xFE0E, 0x039C, 0xFE0E, 0x039D, 0xFE0E, 0x039E,
    0xFE0E, 0x039F, 0xFE0E, 0x03A0, 0xFE0E, 0x03A1, 0xFE0E, 0x03A3, 0xFE0E, 0x03A4, 0xFE0E, 0x03A5,
    0xFE0E, 0x03A6, 0xFE0E, 0x03A7, 0xFE0E, 0x03A8, 0xFE0E, 0x03A9, 0xFE0E, 0x03B1, 0xFE0E, 0x03B2,
    0xFE0E, 0x03B3, 0xFE0E, 0x03B4, 0xFE0E, 0x03B5, 0xFE0E, 0x03B6, 0xFE0E, 0x03B7, 0xFE0E, 0x03B8,
    0xFE0E, 0x03B9, 0xFE0E, 0x03BA, 0xFE0E, 0x03BB, 0xFE0E, 0x03BC, 0xFE0E, 0x03BD, 0xFE0E, 0x03BE,
    0xFE0E, 0x03BF, 0xFE0E, 0x03C0, 0xFE0E, 0x03C1, 0xFE0E, 0x03C3, 0xFE0E, 0x03C4, 0xFE0E,
    0x03C5, 0xFE0E, 0x03C6, 0xFE0E, 0x03C7, 0xFE0E, 0x03C8, 0xFE0E, 0x03C9, 0xFE0E, 0x03AC,
    0xFE0E, 0x0386, 0xFE0E, 0x03AD, 0xFE0E, 0x0388, 0xFE0E, 0x0389, 0xFE0E, 0x03AF, 0xFE0E,
    0x03CA, 0xFE0E, 0x0390, 0xFE0E, 0x038A, 0xFE0E, 0x03CC, 0xFE0E, 0x038C, 0xFE0E, 0x03CD,
    0xFE0E, 0x03B0, 0xFE0E, 0x03CB, 0xFE0E, 0x038E, 0xFE0E, 0x03AB, 0xFE0E, 0x1F70, 0xFE0E,
    0x1F71, 0xFE0E, 0x1F72, 0xFE0E, 0x1F73, 0xFE0E, 0x1F74, 0xFE0E, 0x1F75, 0xFE0E, 0x1F76,
    0xFE0E, 0x1F77, 0xFE0E, 0x1F78, 0xFE0E, 0x1F79, 0xFE0E, 0x1F7A, 0xFE0E, 0x1F7B, 0xFE0E,
    0x1F7C, 0xFE0E, 0x1F7D, 0xFE0E, 0x038F, 0xFE0E, 0x0102, 0xFE0E, 0x00C2, 0xFE0E, 0x00CA,
    0xFE0E, 0x00D4, 0xFE0E, 0x01A0, 0xFE0E, 0x01AF, 0xFE0E, 0x0103, 0xFE0E, 0x00E2, 0xFE0E,
    0x00EA, 0xFE0E, 0x00F4, 0xFE0E, 0x01A1, 0xFE0E, 0x01B0, 0xFE0E, 0x0031, 0xFE0E, 0x0032,
    0xFE0E, 0x0033, 0xFE0E, 0x0034, 0xFE0E, 0x0035, 0xFE0E, 0x0036, 0xFE0E, 0x0037, 0xFE0E,
    0x0038, 0xFE0E, 0x0039, 0xFE0E, 0x0030, 0xFE0E, 0x2018, 0xFE0E, 0x003F, 0xFE0E, 0x2019,
    0xFE0E, 0x201C, 0xFE0E, 0x0021, 0xFE0E, 0x201D, 0xFE0E, 0x0028, 0xFE0E, 0x0025, 0xFE0E,
    0x0029, 0xFE0E, 0x005B, 0xFE0E, 0x0023, 0xFE0E, 0x005D, 0xFE0E, 0x007B, 0xFE0E, 0x0040,
    0xFE0E, 0x007D, 0xFE0E, 0x002F, 0xFE0E, 0x0026, 0xFE0E, 0x005C, 0xFE0E, 0x003C, 0xFE0E,
    0x002D, 0xFE0E, 0x002B, 0xFE0E, 0x00F7, 0xFE0E, 0x00D7, 0xFE0E, 0x003D, 0xFE0E, 0x003E,
    0xFE0E, 0x00AE, 0xFE0E, 0x00A9, 0xFE0E, 0x0024, 0xFE0E, 0x20AC, 0xFE0E, 0x00A3, 0xFE0E,
    0x00A5, 0xFE0E, 0x00A2, 0xFE0E, 0x003A, 0xFE0E, 0x003B, 0xFE0E, 0x002C, 0xFE0E, 0x002E,
    0xFE0E, 0x002A, 0xFE0E,
*/

#if 0
// internal texture MakeNothingsTest(r32 Size, u32 Codepoint) {
//     file FontFile = LoadFile("eb_garamond.ttf");

//     stbtt_fontinfo  Font;
//     stbtt_InitFont(&Font, FontFile.Data, stbtt_GetFontOffsetForIndex(FontFile.Data, 0));

//     glyph Glyph = GetGlyph(&Font, Size, Codepoint);
//     //note: bad idea?

//     texture Result = {0}; {
//         Result.w   = Glyph.Image.w;
//         Result.h   = Glyph.Image.h;
//         Result.Id  = 0;
//     }

//     glGenTextures(1, &Result.Id);
//     glBindTexture(GL_TEXTURE_2D, Result.Id);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Result.w, Result.h, 0,
//                  GL_RGBA, GL_UNSIGNED_BYTE, Glyph.Image.Data);
    
//     FreeMemory(Glyph.Image.Data);
//     FreeFile(FontFile);

//     return Result;
// }
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
