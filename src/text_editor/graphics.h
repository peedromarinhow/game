#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <windows.h>
#include <gl/gl.h>
//todo: how to get rid of these?

#include "lingo.h"
#include "platform.h"
#include "maths.h"
#include "memory.h"

typedef struct _image {
    void *Data;
    i32 w;
    i32 h;
    i32 Format;
} image;

typedef struct _texture {
    u32 Id;
    i32 w;
    i32 h;
    i32 Format;
} texture;

texture TextureFromImage(image Image) {
    texture Result = {0};
    if (Image.Data && Image.w != 0 && Image.h != 0) {
        glGenTextures(1, &Result.Id);
        glBindTexture(GL_TEXTURE_2D, Result.Id);
    }
    Result.w      = Image.w;
    Result.h      = Image.h;
    Result.Format = Image.Format;

    return Result;
}

#define ORIGIN_CENTERED iv2_( 0,  0)
#define ORIGIN_TOPRIGHT iv2_( 1,  1)
#define ORIGIN_BOTRIGHT iv2_( 1, -1)
#define ORIGIN_BOTLEFT  iv2_(-1, -1)
#define ORIGIN_TOPLEFT  iv2_(-1,  1)

void Clear(iv2 Origin, iv2 WindowSize, color Color) {
    glLoadIdentity();
    glViewport(0, 0, WindowSize.w, WindowSize.h);

    r32 a = 2.0f/WindowSize.w;
    r32 b = 2.0f/WindowSize.h;
    r32 Proj[] = {
        a, 0, 0, 0,
        0, b, 0, 0,
        0, 0, 1, 0,
       -1,-1, 0, 1
    };

    glLoadMatrixf(Proj);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(Color.r, Color.g, Color.b, Color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}
//doc:
/* "Origin" is a vector, where the quadrant wich it points to
    represents wich corner of the rect is the origin. <0, 0>
    beeing an origin at the center. "Size" is absolute, mean-
    ing that it is independent from the origin. */
void DrawRect(iv2 Origin, rv2 Pos, rv2 Size, color Color,
              r32 StrokeWidth, color StrokeColor)
{
    rv2 TopLeft     = rv2_(0, 0);
    rv2 TopRight    = rv2_(0, 0);
    rv2 BottomRight = rv2_(0, 0);
    rv2 BottomLeft  = rv2_(0, 0);

    glColor4f(Color.r, Color.g, Color.b, Color.a);
    if (Origin.x == 0 && Origin.y == 0) {
        TopLeft     = rv2_(Pos.x - Size.w/2.0f, Pos.y - Size.h/2.0f);
        TopRight    = rv2_(Pos.x + Size.w/2.0f, Pos.y - Size.h/2.0f);
        BottomRight = rv2_(Pos.x + Size.w/2.0f, Pos.y + Size.h/2.0f);
        BottomLeft  = rv2_(Pos.x - Size.w/2.0f, Pos.y + Size.h/2.0f);
    }
    else
    if (Origin.x == 1 && Origin.y == 1) {
        TopLeft     = rv2_(Pos.x - Size.w, Pos.y);
        TopRight    = rv2_(Pos.x, Pos.y);
        BottomRight = rv2_(Pos.x, Pos.y + Size.h);
        BottomLeft  = rv2_(Pos.x - Size.w, Pos.y + Size.h);
    }
    else
    if (Origin.x == 1 && Origin.y == -1) {
        TopLeft     = rv2_(Pos.x - Size.w, Pos.y - Size.h);
        TopRight    = rv2_(Pos.x, Pos.y - Size.h);
        BottomRight = rv2_(Pos.x, Pos.y);
        BottomLeft  = rv2_(Pos.x - Size.w, Pos.y);
    }
    else
    if (Origin.x == -1 && Origin.y == -1) {
        TopLeft     = rv2_(Pos.x, Pos.y - Size.h);
        TopRight    = rv2_(Pos.x + Size.w, Pos.y - Size.h);
        BottomRight = rv2_(Pos.x + Size.w, Pos.y);
        BottomLeft  = rv2_(Pos.x, Pos.y);
    }
    else
    if (Origin.x == -1 && Origin.y == 1) {
        TopLeft     = rv2_(Pos.x, Pos.y);
        TopRight    = rv2_(Pos.x + Size.w, Pos.y);
        BottomRight = rv2_(Pos.x + Size.w, Pos.y + Size.h);
        BottomLeft  = rv2_(Pos.x, Pos.y + Size.h);
    }
    else {
        return;
    }

    if (Color.a != (0/255.f)) {
        glBegin(GL_POLYGON); {
            glColor4f(Color.r, Color.g, Color.b, Color.a);
            glVertex2f(TopLeft.x, TopLeft.y);
            glVertex2f(TopRight.x, TopRight.y);
            glVertex2f(BottomRight.x, BottomRight.y);
            glVertex2f(BottomLeft.x, BottomLeft.y);
        } glEnd();
    }

    if (StrokeWidth > 0) {
        glLineWidth(StrokeWidth);
        glBegin(GL_LINE_LOOP); {
            glColor4f(StrokeColor.r, StrokeColor.g, StrokeColor.b, StrokeColor.a);
            glVertex2f(TopLeft.x, TopLeft.y);
            glVertex2f(TopRight.x, TopRight.y);
            glVertex2f(BottomRight.x, BottomRight.y);
            glVertex2f(BottomLeft.x, BottomLeft.y);
        } glEnd();
    }
}

void DrawLine(rv2 a, rv2 b, r32 StrokeWidth, color Color) {
    glLineWidth(StrokeWidth);
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINES); {
        glColor4f(Color.r, Color.g, Color.b, Color.a);
        glVertex2f(a.x, a.y);
        glVertex2f(b.x, b.y);
    } glEnd();
}

void gDrawTexture(texture Texture, rv2 Center, rv2 Size, color Tint) {
    glBindTexture(GL_TEXTURE_2D, Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
        glTexCoord2f(0, 0);
        glVertex2f  (Center.x - Size.w/2.0f, Center.y - Size.h/2.0f);
        glTexCoord2f(1, 0);
        glVertex2f  (Center.x + Size.w/2.0f, Center.y - Size.h/2.0f);
        glTexCoord2f(1, 1);
        glVertex2f  (Center.x + Size.w/2.0f, Center.y + Size.h/2.0f);
        glTexCoord2f(0, 1);
        glVertex2f  (Center.x - Size.w/2.0f, Center.y + Size.h/2.0f);
    } glEnd();
    glDisable(GL_TEXTURE_2D);
}

#endif//GRAPHICS_H







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

u32 GetNextCodepoint(const char *text, unsigned int *bytesProcessed) {
/*
    UTF8 specs from https://www.ietf.org/rfc/rfc3629.txt

    Char. number range  |        UTF-8 octet sequence
      (hexadecimal)    |              (binary)
    --------------------+---------------------------------------------
    0000 0000-0000 007F | 0xxxxxxx
    0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
    // NOTE: on decode errors we return as soon as possible

    int code = 0x3f;   // Codepoint (defaults to '?')
    int octet = (unsigned char)(text[0]); // The first UTF8 octet
    *bytesProcessed = 1;

    if (octet <= 0x7f)
    {
        // Only one octet (ASCII range x00-7F)
        code = text[0];
    }
    else if ((octet & 0xe0) == 0xc0)
    {
        // Two octets
        // [0]xC2-DF    [1]UTF8-tail(x80-BF)
        unsigned char octet1 = text[1];

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

        if ((octet >= 0xc2) && (octet <= 0xdf))
        {
            code = ((octet & 0x1f) << 6) | (octet1 & 0x3f);
            *bytesProcessed = 2;
        }
    }
    else if ((octet & 0xf0) == 0xe0)
    {
        // Three octets
        unsigned char octet1 = text[1];
        unsigned char octet2 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; } // Unexpected sequence

        /*
            [0]xE0    [1]xA0-BF       [2]UTF8-tail(x80-BF)
            [0]xE1-EC [1]UTF8-tail    [2]UTF8-tail(x80-BF)
            [0]xED    [1]x80-9F       [2]UTF8-tail(x80-BF)
            [0]xEE-EF [1]UTF8-tail    [2]UTF8-tail(x80-BF)
        */

        if (((octet == 0xe0) && !((octet1 >= 0xa0) && (octet1 <= 0xbf))) ||
            ((octet == 0xed) && !((octet1 >= 0x80) && (octet1 <= 0x9f)))) { *bytesProcessed = 2; return code; }

        if ((octet >= 0xe0) && (0 <= 0xef))
        {
            code = ((octet & 0xf) << 12) | ((octet1 & 0x3f) << 6) | (octet2 & 0x3f);
            *bytesProcessed = 3;
        }
    }
    else if ((octet & 0xf8) == 0xf0)
    {
        // Four octets
        if (octet > 0xf4) return code;

        unsigned char octet1 = text[1];
        unsigned char octet2 = '\0';
        unsigned char octet3 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; }  // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; }  // Unexpected sequence

        octet3 = text[3];

        if ((octet3 == '\0') || ((octet3 >> 6) != 2)) { *bytesProcessed = 4; return code; }  // Unexpected sequence

        /*
            [0]xF0       [1]x90-BF       [2]UTF8-tail  [3]UTF8-tail
            [0]xF1-F3    [1]UTF8-tail    [2]UTF8-tail  [3]UTF8-tail
            [0]xF4       [1]x80-8F       [2]UTF8-tail  [3]UTF8-tail
        */

        if (((octet == 0xf0) && !((octet1 >= 0x90) && (octet1 <= 0xbf))) ||
            ((octet == 0xf4) && !((octet1 >= 0x80) && (octet1 <= 0x8f)))) { *bytesProcessed = 2; return code; } // Unexpected sequence

        if (octet >= 0xf0)
        {
            code = ((octet & 0x7) << 18) | ((octet1 & 0x3f) << 12) | ((octet2 & 0x3f) << 6) | (octet3 & 0x3f);
            *bytesProcessed = 4;
        }
    }

    if (code > 0x10ffff) code = 0x3f;     // Codepoints after U+10ffff are invalid

    return code;
}

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

//note: stolen from raylib
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
            if (Result.Chars[i].Codepoint == '\t') {
                Result.Rects[i].w = Result.Chars[i].Advance;
            }
            if (Result.Chars[i].Codepoint == '\r') {
                Result.Rects[i].w = Result.Chars[i].Advance;
            }
            if (Result.Chars[i].Codepoint == '\n') {
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

//note: stolen from raylib
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

void DrawText_(font *Font, c8 *Text, rv2 Pos, f32 Size, f32 CharSpacing,
               f32 LineSpacing, color Tint)
{
    f32 ScaleFactor = Size/Font->Size;
    f32 CharOffset  = 0;
    f32 LineOffset  = 0;

    glBindTexture(GL_TEXTURE_2D, Font->Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
        i32 i = 0;
        while (Text[i] != '\0') {
            i32 Index     = GetGlyphIndex(*Font, Text[i]);
            i32 Codepoint = Font->Chars[Index].Codepoint;
            i32 OffY      = Font->Chars[Index].OffY;
            i32 OffX      = Font->Chars[Index].OffX;

            if (Text[i] == '\t') {
                CharOffset += Font->Size * 1.5f;
                Index = GetGlyphIndex(*Font, ' ');
            }

            if (Text[i] == '\n') {
                LineOffset += Font->Size + (LineSpacing * ScaleFactor);
                CharOffset = 0;
                Index = GetGlyphIndex(*Font, ' ');
            }

            if (Text[i] == '\r') {
                Index = GetGlyphIndex(*Font, ' ');
            }

            rectf32 Rect = Font->Rects[Index];
            f32 w        = Font->Texture.w;
            f32 h        = Font->Texture.h;

            glTexCoord2f(Rect.x/w, Rect.y/h);
            glVertex2f  (Pos.x + CharOffset + OffX,
                         Pos.y - LineOffset - OffY);

            glTexCoord2f((Rect.x + Rect.w)/w, Rect.y/h);
            glVertex2f  (Pos.x + CharOffset + Rect.w + OffX,
                         Pos.y - LineOffset - OffY);

            glTexCoord2f((Rect.x + Rect.w)/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharOffset + Rect.w + OffX,
                         Pos.y - LineOffset - Rect.h - OffY);

            glTexCoord2f(Rect.x/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharOffset + OffX,
                         Pos.y - LineOffset - Rect.h - OffY);

            CharOffset += (Font->Chars[Index].Advance * ScaleFactor) +
                          (CharSpacing * ScaleFactor);
            i++;
        }
    } glEnd();
    glDisable(GL_TEXTURE_2D);
}

void gDrawTextLen(font *Font, r32 Size, rv2 Pos,
                  f32 CharSpacing, f32 LineSpacing,
                  c8 *Text, u32 Len, color Tint)
{
    f32 ScaleFactor = Size/Font->Size;
    f32 CharOffset  = 0;
    f32 LineOffset  = 0;

    glBindTexture(GL_TEXTURE_2D, Font->Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
        for (u32 i = 0; i < Len; i++) {
            i32 Index     = GetGlyphIndex(*Font, Text[i]);
            i32 Codepoint = Font->Chars[Index].Codepoint;
            i32 OffY      = Font->Chars[Index].OffY;
            i32 OffX      = Font->Chars[Index].OffX;
            glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);

            if (Text[i] == '\t') {
                CharOffset += (Font->Size + Font->Size/2);
                continue; //note: skips the rest, so the caracter is not drawn
            }

            if (Text[i] == '\n') {
                LineOffset += Font->Size + (LineSpacing * ScaleFactor);
                CharOffset = 0;
                continue; //note: skips the rest, so the caracter is not drawn
            }

            rectf32 Rect = Font->Rects[Index];
            f32 w        = Font->Texture.w;
            f32 h        = Font->Texture.h;

            glTexCoord2f(Rect.x/w, Rect.y/h);
            glVertex2f  (Pos.x + CharOffset + OffX,
                         Pos.y + LineOffset - OffY);

            glTexCoord2f((Rect.x + Rect.w)/w, Rect.y/h);
            glVertex2f  (Pos.x + CharOffset + Rect.w + OffX,
                         Pos.y + LineOffset - OffY);

            glTexCoord2f((Rect.x + Rect.w)/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharOffset + Rect.w + OffX,
                         Pos.y + LineOffset + Rect.h - OffY);

            glTexCoord2f(Rect.x/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharOffset + OffX,
                         Pos.y + LineOffset + Rect.h - OffY);

            CharOffset += (Font->Chars[Index].Advance * ScaleFactor) +
                          (CharSpacing * ScaleFactor);
        }
    } glEnd();
    glDisable(GL_TEXTURE_2D);
}

#endif//FONTS_H
