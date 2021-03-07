#include <windows.h>
#include <gl/gl.h>
//todo: how to get rid of these?

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "lingo.h"
#include "platform.h"
#include "maths.h"
#include "memory.h"

typedef struct _rect {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
} rect;

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

#if A

texture TextureFromImage(image Image) {
    texture Result = {0};
    if (Image.Data && Image.w != 0 && Image.h != 0) {
    }
    Result.w      = Image.w;
    Result.h      = Image.h;
    Result.Format = Image.Format;

    return Result;
}

image GenerateFontAtlas(font *Font, i32 Padding) {
    image Result = {0};

    Font->Rects = AllocateMemory(Font->NumberOfChars * sizeof(rect));

    f32 Area = 0;
    for (u32 i = 0;
             i < Font->NumberOfChars;
             i++)
    {
        Area += (Font->Chars[i].Image.w + 2 * Padding) * (Font->Chars[i].Image.h + 2 * Padding);
    }

    f32 RequiredArea = 0;
    for (u32 i = 0; i < Font->NumberOfChars; i++) RequiredArea += ((Font->Chars[i].Image.w + 2*Padding)*(Font->Chars[i].Image.h + 2*Padding));
    float GuessSize = sqrtf(RequiredArea)*1.3f;
    int ImageSize = (int)powf(2, ceilf(logf((float)GuessSize)/logf(2)));  // Calculate next POT
    
    Result.w      = ImageSize;
    Result.h      = ImageSize;
    Result.Data   = AllocateMemory(Result.w * Result.h * sizeof(color4b));
    Result.Format = 1; //uncompressed grayscale

    i32 OffsetX = Padding;
    i32 OffsetY = Padding;

    for (u32 i = 0; i < Font->NumberOfChars; i++) {
        // for (i32 y = 0; y < Font->Chars[i].Image.h; y++) {
        //     for (i32 x = 0; x < Font->Chars[i].Image.w; x++) {
        //         ((u8 *)Result.Data)[(OffsetY + y) * Result.w + (OffsetX + x)] =
        //             ((u8 *)Font->Chars[i].Image.Data)[y * Font->Chars[i].Image.w + x];
        //     }
        // }
        for (i32 y = 0; y < Font->Chars[i].Image.h; y++) {
            for (i32 x = 0; x < Font->Chars[i].Image.w; x++) {
                ((color4b *)Result.Data)[(OffsetY + y) * Result.w + (OffsetX + x)].r =
                    ((u8 *)Font->Chars[i].Image.Data)[y * Font->Chars[i].Image.w + x];
                ((color4b *)Result.Data)[(OffsetY + y) * Result.w + (OffsetX + x)].a = 255;
            }
        }

        Font->Rects[i].x = (f32)OffsetX;
        Font->Rects[i].y = (f32)OffsetY;
        Font->Rects[i].w = (f32)Font->Chars[i].Image.w;
        Font->Rects[i].h = (f32)Font->Chars[i].Image.h;

        OffsetX += Font->Chars[i].Image.w + 2 * Padding;

        if (OffsetX >= (Result.w - Font->Chars[i].Image.w - Padding)) {
            OffsetX  = Padding;
            OffsetY += (Font->Size + 2 * Padding);

            if (OffsetY > (Result.h - Font->Size - Padding))
                break;
        }
    }

    u8 *GrayscaleData = (u8 *)AllocateMemory(Result.w * Result.h * sizeof(u8) * 2);
    for (i32 i = 0, k = 0; i < Result.w * Result.h; i++, k += 2) {
        GrayscaleData[k]     = 255;
        GrayscaleData[k + 1] = ((u8 *)Result.Data)[i];
    }

    FreeMemory(Result.Data);
    Result.Data   = GrayscaleData;
    Result.Format = 1; //uncompressed grayscale

    return Result;
}
font_character *LoadFontChars(c8* Filename, i32 Size, u32 NumberOfChars) {
    font_character *Result = NULL;
    file File = FileLoad(Filename);
    if (File.Data) {
        stbtt_fontinfo FontInfo = {0};
        if (stbtt_InitFont(&FontInfo, File.Data, 0)) {
            f32 CharacterScaleFactor = stbtt_ScaleForPixelHeight(&FontInfo, (f32)Size);

            i32 Ascender;
            i32 Descender;
            i32 GapBetweenLines;
            stbtt_GetFontVMetrics(&FontInfo, &Ascender, &Descender, &GapBetweenLines);

            NumberOfChars = (NumberOfChars > 0)? NumberOfChars : 95;

            Result = (font_character *)AllocateMemory(NumberOfChars * sizeof(font_character));

            for (u32 i = 0;
                     i < NumberOfChars;
                   ++i)
            {
                i32 CharacterW = 0;
                i32 CharacterH = 0;
                i32 Codepoint  = i + 32;

                Result[i].Codepoint  = Codepoint;
                Result[i].Image.Data = stbtt_GetCodepointBitmap(&FontInfo, CharacterScaleFactor, CharacterScaleFactor, Codepoint,
                                                                             &CharacterW, &CharacterH,
                                                                             &Result[i].OffsetX,
                                                                             &Result[i].OffsetY);

                stbtt_GetCodepointHMetrics(&FontInfo, Codepoint, &Result[i].Advance, NULL);
                Result[i].Advance = (i32)((f32)Result[i].Advance * Size);

                Result[i].Image.w      = CharacterW;
                Result[i].Image.h      = CharacterH;
                Result[i].Image.Format = 1; //uncompressed grayscale

                Result[i].OffsetY += (i32)((f32)Ascender * Size);

                if (Codepoint == 32) {
                    image BlankImage = {0}; {
                        BlankImage.Data   = AllocateMemory(Result[i].Advance * Size * sizeof(color4f));
                        BlankImage.w      = Result[i].Advance;
                        BlankImage.h      = Size;
                        BlankImage.Format = 1; //uncompressed grayscale
                    }
                    Result[i].Image = BlankImage;
                }
            }
        }
        FileFree(File);
    }
    return Result;
}
#endif

// texture TextureFromImage(image Image) {
//     texture Result = {0};
//     if (Image.Data && Image.w != 0 && Image.h != 0) {
//         glGenTextures(1, &Result.Id);
//         glBindTexture(GL_TEXTURE_2D, Result.Id);
//     }
//     Result.w      = Image.w;
//     Result.h      = Image.h;
//     Result.Format = Image.Format;

//     return Result;
// }

// void gDrawTexture(texture Texture, rect SourceRect, rect DestRect, rv2 Origin, f32 Angle, color4f Tint) {
//     if (Texture.Id > 0) {
//         f32 w = Texture.w;
//         f32 h = Texture.h;

//         b32 FlipX = 0;

//         if (SourceRect.w < 0) {
//             FlipX = 1;
//             SourceRect.w *= -1;
//         }
//         if (SourceRect.h < 0) {
//             SourceRect.y -= SourceRect.h;
//         }

//         glEnable(GL_TEXTURE_2D);
//         glBindTexture(GL_TEXTURE_2D, Texture.Id);
//         glTranslatef(DestRect.x, DestRect.y, 0.0f);
//         glRotatef(Angle, 0.0f, 0.0f, 1.0f);
//         glTranslatef(-Origin.x, -Origin.y, 0.0f);

//         glBegin(GL_QUADS); {
//             glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
//             glNormal3f(-Origin.x, -Origin.y, 0.0f);

//             // Bottom-left corner for texture and quad
//             if (FlipX) glTexCoord2f((SourceRect.x + SourceRect.w)/w, SourceRect.y/h);
//             else       glTexCoord2f(SourceRect.x/w, SourceRect.y/h);
//             glVertex2f(0.0f, 0.0f);

//             // Bottom-right corner for texture and quad
//             if (FlipX) glTexCoord2f((SourceRect.x + SourceRect.w)/w, (SourceRect.y + SourceRect.h)/h);
//             else       glTexCoord2f(SourceRect.x/w, (SourceRect.y + SourceRect.h)/h);
//             glVertex2f(0.0f, DestRect.h);

//             // Top-right corner for texture and quad
//             if (FlipX) glTexCoord2f(SourceRect.x/w, (SourceRect.y + SourceRect.h)/h);
//             else       glTexCoord2f((SourceRect.x + SourceRect.w)/w, (SourceRect.y + SourceRect.h)/h);
//             glVertex2f(DestRect.w, DestRect.h);

//             // Top-left corner for texture and quad
//             if (FlipX) glTexCoord2f(SourceRect.x/w, SourceRect.y/h);
//             else       glTexCoord2f((SourceRect.x + SourceRect.w)/w, SourceRect.y/h);
//             glVertex2f(DestRect.w, 0.0f);

//             // //bottom left
//             // if (FlipX) glTexCoord2f((SourceRect.x + SourceRect.w) / w, SourceRect.y / h);
//             // else       glTexCoord2f(SourceRect.x / w, SourceRectt.y / h);
//             // glVertex2f(0.0f, 0.0f);

//             // //bottom right
//             // if (FlipX) glTexCoord2f((SourceRect.x + SourceRect.w) / w, (SourceRect.y + SourceRect.h) / h);
//             // else       glTexCoord2f(SourceRect.x / w, (SourceRectt.y + SourceRectt.h) / h);
//             // glVertex2f(0.0f, DestRect.h);

//             // //top right
//             // if (FlipX) glTexCoord2f(SourceRect.x / w, (SourceRect.y + SourceRect.h) / h);
//             // else       glTexCoord2f((SourceRect.x + SourceRectt.w) / w, (SourceRectt.y + SourceRectt.h) / h);
//             // glVertex2f(DestRect.w, DestRect.h);

//             // //top left
//             // if (FlipX) glTexCoord2f(SourceRect.x / w, SourceRect.y / h);
//             // else       glTexCoord2f((SourceRect.x + SourceRectt.w) / w, SourceRectt.y / h);
//             // glVertex2f(DestRect.w, 0.0f);
//         } glEnd();
//         glDisable(GL_TEXTURE_2D);
//         glBindTexture(GL_TEXTURE_2D, 0);
//     }
// }

// void UnloadFont(font Font) {
//     for (u32 i = 0; i < Font.NumberOfChars; i++)
//         MemFree(Font.Chars[i].Image.Data);
// }

// int GetNextCodepoint(const char *text, int *bytesProcessed)
// {
// /*
//     UTF8 specs from https://www.ietf.org/rfc/rfc3629.txt

//     Char. number range  |        UTF-8 octet sequence
//       (hexadecimal)    |              (binary)
//     --------------------+---------------------------------------------
//     0000 0000-0000 007F | 0xxxxxxx
//     0000 0080-0000 07FF | 110xxxxx 10xxxxxx
//     0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
//     0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// */
//     // NOTE: on decode errors we return as soon as possible

//     int code = 0x3f;   // Codepoint (defaults to '?')
//     int octet = (unsigned char)(text[0]); // The first UTF8 octet
//     *bytesProcessed = 1;

//     if (octet <= 0x7f)
//     {
//         // Only one octet (ASCII range x00-7F)
//         code = text[0];
//     }
//     else if ((octet & 0xe0) == 0xc0)
//     {
//         // Two octets
//         // [0]xC2-DF    [1]UTF8-tail(x80-BF)
//         unsigned char octet1 = text[1];

//         if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

//         if ((octet >= 0xc2) && (octet <= 0xdf))
//         {
//             code = ((octet & 0x1f) << 6) | (octet1 & 0x3f);
//             *bytesProcessed = 2;
//         }
//     }
//     else if ((octet & 0xf0) == 0xe0)
//     {
//         // Three octets
//         unsigned char octet1 = text[1];
//         unsigned char octet2 = '\0';

//         if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

//         octet2 = text[2];

//         if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; } // Unexpected sequence

//         /*
//             [0]xE0    [1]xA0-BF       [2]UTF8-tail(x80-BF)
//             [0]xE1-EC [1]UTF8-tail    [2]UTF8-tail(x80-BF)
//             [0]xED    [1]x80-9F       [2]UTF8-tail(x80-BF)
//             [0]xEE-EF [1]UTF8-tail    [2]UTF8-tail(x80-BF)
//         */

//         if (((octet == 0xe0) && !((octet1 >= 0xa0) && (octet1 <= 0xbf))) ||
//             ((octet == 0xed) && !((octet1 >= 0x80) && (octet1 <= 0x9f)))) { *bytesProcessed = 2; return code; }

//         if ((octet >= 0xe0) && (0 <= 0xef))
//         {
//             code = ((octet & 0xf) << 12) | ((octet1 & 0x3f) << 6) | (octet2 & 0x3f);
//             *bytesProcessed = 3;
//         }
//     }
//     else if ((octet & 0xf8) == 0xf0)
//     {
//         // Four octets
//         if (octet > 0xf4) return code;

//         unsigned char octet1 = text[1];
//         unsigned char octet2 = '\0';
//         unsigned char octet3 = '\0';

//         if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; }  // Unexpected sequence

//         octet2 = text[2];

//         if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; }  // Unexpected sequence

//         octet3 = text[3];

//         if ((octet3 == '\0') || ((octet3 >> 6) != 2)) { *bytesProcessed = 4; return code; }  // Unexpected sequence

//         /*
//             [0]xF0       [1]x90-BF       [2]UTF8-tail  [3]UTF8-tail
//             [0]xF1-F3    [1]UTF8-tail    [2]UTF8-tail  [3]UTF8-tail
//             [0]xF4       [1]x80-8F       [2]UTF8-tail  [3]UTF8-tail
//         */

//         if (((octet == 0xf0) && !((octet1 >= 0x90) && (octet1 <= 0xbf))) ||
//             ((octet == 0xf4) && !((octet1 >= 0x80) && (octet1 <= 0x8f)))) { *bytesProcessed = 2; return code; } // Unexpected sequence

//         if (octet >= 0xf0)
//         {
//             code = ((octet & 0x7) << 18) | ((octet1 & 0x3f) << 12) | ((octet2 & 0x3f) << 6) | (octet3 & 0x3f);
//             *bytesProcessed = 4;
//         }
//     }

//     if (code > 0x10ffff) code = 0x3f;     // Codepoints after U+10ffff are invalid

//     return code;
// }

// int GetGlyphIndex(font Font, i32 codepoint)
// {
// #define TEXT_CHARACTER_NOTFOUND     63      // Character: '?'
// #define UNORDERED_CHARSET
// #if defined(UNORDERED_CHARSET)
//     int index = TEXT_CHARACTER_NOTFOUND;
//     for (u32 i = 0; i < Font.NumberOfChars; i++)
//     {
//         if (Font.Chars[i].Codepoint == codepoint)
//         {
//             index = i;
//             break;
//         }
//     }

//     return index;
// #else
//     return (codepoint - 32);
// #endif
// }

// void gDrawText(font Font, c8 *Text, rv2 Pos, f32 Size, f32 Spacing, color4f Color) {
//     i32 Length = ArrayCount(Text);

//     i32 OffsetY = 0;
//     f32 OffsetX = 0.0f;
//     f32 ScaleFactor = Size / Font.Size;

//     for (i32 i = 0; i < Length; i++) {
//         i32 CodepointByteCount = 0;
//         i32 Codepoint          = GetNextCodepoint(&Text[i], &CodepointByteCount);
//         i32 Index              = GetGlyphIndex(Font, Codepoint);

//         if (Codepoint == 0x3F)
//             CodepointByteCount = 1;
        
//         if (Codepoint == '\n') {
//             OffsetY += (i32)((Font.Size + Font.Size / 2)* ScaleFactor );
//             OffsetX = 0.0f;
//         }
//         else {
//             if (Codepoint != ' ' && Codepoint != '\t') {
//                 r32 X = Pos.y + OffsetY + Font.Chars[i].OffsetY * ScaleFactor;
//                 rect Rect = {Pos.x + OffsetX + Font.Chars[i].OffsetX * ScaleFactor,
//                              Pos.y + OffsetY + Font.Chars[i].OffsetY * ScaleFactor,
//                              Font.Rects[i].w * ScaleFactor,
//                              Font.Rects[i].h * ScaleFactor};
//                 gDrawTexture(Font.Texture, Font.Rects[i], Rect, Rv2(0, 0), 0.0f, Color);
//             }

//             if (Font.Chars[i].Advance == 0)
//                 OffsetX += ((f32)Font.Rects[i].w * ScaleFactor + Spacing);
//             else
//                 OffsetX += ((f32)Font.Chars[i].Advance * ScaleFactor + Spacing);
//         }

//         i += (CodepointByteCount - 1);
//     }
// }

void gBegin(rv2 Shift, iv2 Size, color4f Color) {
    glLoadIdentity();
    glViewport(Shift.x, Shift.y, Size.w, Size.h);
    glClearColor(Color.r, Color.g, Color.b, Color.a);
    glClear(GL_COLOR_BUFFER_BIT);
    r32 a = 2.0f/Size.w;
    r32 b = 2.0f/Size.h;
    r32 Proj[] = {
         a,  0,  0,  0,
         0, -b,  0,  0,
         0,  0,  1,  0,
        -1,  1,  0,  1
    };
    glLoadMatrixf(Proj);
}

void gRectFromCenter(rv2 Pos, rv2 Size, color4f Color) {
    glBegin(GL_POLYGON); {
        glColor4f(Color.r, Color.g, Color.b, Color.a);
        glVertex2f(Pos.x - Size.w/2.0f, Pos.y - Size.h/2.0f);
        glVertex2f(Pos.x + Size.w/2.0f, Pos.y - Size.h/2.0f);
        glVertex2f(Pos.x + Size.w/2.0f, Pos.y + Size.h/2.0f);
        glVertex2f(Pos.x - Size.w/2.0f, Pos.y + Size.h/2.0f);
    } glEnd();
}

r32 f(r32 t, r32 Duration, r32 Scale) {
    return -(Scale / 2.0f)*(Cos((PI32 * t) / Duration) - 1);
}

void DrawRectangleFromCenter(rv2 Center, rv2 Size) {
    glBegin(GL_POLYGON); {
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(Center.x - Size.w/2.0f, Center.y - Size.h/2.0f);
        glVertex2f(Center.x + Size.w/2.0f, Center.y - Size.h/2.0f);
        glVertex2f(Center.x + Size.w/2.0f, Center.y + Size.h/2.0f);
        glVertex2f(Center.x - Size.w/2.0f, Center.y + Size.h/2.0f);
    } glEnd();
}

void DrawRectangleStrokeFromCenter(rv2 Center, rv2 Size, r32 StrokeWidth) {
    glLineWidth(StrokeWidth);
    glBegin(GL_LINE_LOOP); {
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(Center.x - Size.w/2.0f, Center.y - Size.h/2.0f);
        glVertex2f(Center.x + Size.w/2.0f, Center.y - Size.h/2.0f);
        glVertex2f(Center.x + Size.w/2.0f, Center.y + Size.h/2.0f);
        glVertex2f(Center.x - Size.w/2.0f, Center.y + Size.h/2.0f);
    } glEnd();
}

void DrawFilledCircle(rv2 Center, r32 Radius, u32 IterationCount){
	GLfloat TwoPi = 2.0f * PI32;
	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(Center.x, Center.y); // center of circle
		for(u32 IterationIndex = 0; IterationIndex <= IterationCount; IterationIndex++) { 
			glVertex2f(
		        Center.x + (Radius * Cos(IterationIndex * TwoPi / IterationCount)), 
			    Center.y + (Radius * Sin(IterationIndex * TwoPi / IterationCount))
			);
		}
	glEnd();
}

#pragma pack(push, 1)
typedef struct _bitmap_header {
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    i32 Width;
    i32 Height;
    u16 Planes;
    u16 BitsPerPixel;
} bitmap_header;
#pragma pack(pop)

void DrawTexture() {
    // glBindTexture(GL_TEXTURE_2D, Texture.Id);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Texture.w, Texture.h, 0,
    //              GL_BGR_EXT, GL_UNSIGNED_BYTE, 0/*Texture.Pixels*/);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);
    // glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glBegin(GL_POLYGON);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f  (-1, -1);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f  ( 1, -1);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f  ( 1, 1);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f  (-1, 1);
    glEnd();
}

#endif//GRAPHICS_H
