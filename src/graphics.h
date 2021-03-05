#include <windows.h>
#include <gl/gl.h>
//todo: how to get rid of these?

#include "lingo.h"
#include "platform.h"
#include "maths.h"
#include "memory.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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

typedef struct _font_character {
    i32   Codepoint;
    i32   OffsetX;
    i32   OffsetY;
    i32   Advance;
    image Image;
} font_character;

typedef struct _font {
    i32             Size;
    u32             NumberOfChars;
    texture         Texture;
    rect           *CharRectangles;
    font_character *Characters;
} font;

font_character *LoadFontCharacters(c8* Filename, i32 Size, u32 NumberOfCharacters) {
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

            NumberOfCharacters = (NumberOfCharacters > 0)? NumberOfCharacters : 95;

            Result = (font_character *)MemAlloc(NumberOfCharacters * sizeof(font_character));

            for (u32 CharacterIndex = 0;
                     CharacterIndex < NumberOfCharacters;
                   ++CharacterIndex)
            {
                i32 CharacterW = 0;
                i32 CharacterH = 0;
                i32 Codepoint  = CharacterIndex + 32;

                Result[CharacterIndex].Codepoint  = Codepoint;
                Result[CharacterIndex].Image.Data = stbtt_GetCodepointBitmap(&FontInfo, Size, Size,Codepoint,
                                                                             &CharacterW, &CharacterH,
                                                                             &Result[CharacterIndex].OffsetX,
                                                                             &Result[CharacterIndex].OffsetY);

                stbtt_GetCodepointHMetrics(&FontInfo, Codepoint, &Result[CharacterIndex].Advance, NULL);
                Result[CharacterIndex].Advance = (i32)((f32)Result[CharacterIndex].Advance * Size);

                Result[CharacterIndex].Image.w      = CharacterW;
                Result[CharacterIndex].Image.h      = CharacterH;
                Result[CharacterIndex].Image.Format = 1; //uncompressed grayscale

                Result[CharacterIndex].OffsetY = (i32)((f32)Ascender * Size);

                if (Codepoint == 32) {
                    image BlankImage = {0}; {
                        BlankImage.Data   = MemAlloc(Result[CharacterIndex].Advance * Size * sizeof(color4f));
                        BlankImage.w      = Result[CharacterIndex].Advance;
                        BlankImage.h      = Size;
                        BlankImage.Format = 1; //uncompressed grayscale
                    }
                    Result[CharacterIndex].Image = BlankImage;
                }
            }
        }
        FileFree(File);
    }
    return Result;
}

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

image GenerateFontAtlas(font *Font, i32 Padding) {
    image Result = {0};

    Font->CharRectangles = NULL;

    u32   NumberOfCharacters = (Font->NumberOfChars > 0)? Font->NumberOfChars : 95;   
    rect *Rectangles         = MemAlloc(NumberOfCharacters * sizeof(rect));

    f32 Area = 0;
    for (u32 CharacterIndex = 0;
             CharacterIndex < NumberOfCharacters;
             CharacterIndex++)
    {
        Area += (Font->Characters[CharacterIndex].Image.w + 2 * Padding) * (Font->Characters[CharacterIndex].Image.h + 2 * Padding);
    }

    i32 ImageWH = (i32)powf(2, ceilf(logf(sqrtf(Area)*1.3f)/logf(2)));
    
    Result.w      = ImageWH;
    Result.h      = ImageWH;
    Result.Data   = MemAlloc(Result.w * Result.h);
    Result.Format = 1; //uncompressed grayscale

    i32 OffsetX = Padding;
    i32 OffsetY = Padding;

    for (u32 CharacterIndex = 0;
             CharacterIndex < NumberOfCharacters;
             CharacterIndex++)
    {
        for (i32 y = 0; y < Font->Characters[CharacterIndex].Image.h; y++) {
            for (i32 x = 0; x < Font->Characters[CharacterIndex].Image.w; x++) {
                ((u8 *)Result.Data)[(OffsetY + y) * Result.w + (OffsetX + x)] =
                    ((u8 *)Font->Characters[CharacterIndex].Image.Data)[y * Font->Characters[CharacterIndex].Image.w + x];
            }
        }

        Font->CharRectangles[CharacterIndex].x = (f32)OffsetX;
        Font->CharRectangles[CharacterIndex].y = (f32)OffsetY;
        Font->CharRectangles[CharacterIndex].w = (f32)Font->Characters[CharacterIndex].Image.w;
        Font->CharRectangles[CharacterIndex].h = (f32)Font->Characters[CharacterIndex].Image.h;

        OffsetX += Font->Characters[CharacterIndex].Image.w + 2 * Padding;

        if (OffsetX >= (Result.w = Font->Characters[CharacterIndex].Image.w - Padding)) {
            OffsetX  = Padding;
            OffsetY += (Font->Size + 2 * Padding);

            if (OffsetY > (Result.h - Font->Size - Padding))
                break;
        }
    }

    u8 *GrayscaleData = (u8 *)MemAlloc(Result.w * Result.h * sizeof(u8) * 2);
    for (i32 i = 0, k = 0; i < Result.w * Result.h; i++, k += 2) {
        GrayscaleData[k]     = 255;
        GrayscaleData[k + 1] = ((u8 *)Result.Data)[i];
    }

    MemFree(Result.Data);
    Result.Data   = GrayscaleData;
    Result.Format = 1; //uncompressed grayscale
    Font->CharRectangles = Rectangles;

    return Result;
}

font LoadFont(c8 *Filename, i32 Size, u32 NumberOfCharacters) {
    font Result = {0}; {
        Result.Size          = Size;
        Result.NumberOfChars = NumberOfCharacters;
        Result.Characters    = LoadFontCharacters(Filename, Size, NumberOfCharacters);
    }
    
    if (Result.Characters) {
        image Atlas    = GenerateFontAtlas(&Result, 2);
        Result.Texture = TextureFromImage(Atlas);
        MemFree(Atlas.Data);
    }

    return Result;
}

void gBegin(rv2 Shift, iv2 Size, color4f Color) {
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

#if 0
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

void DrawTexture(texture Texture) {
    glBindTexture(GL_TEXTURE_2D, Texture.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Texture.w, Texture.h, 0,
                 GL_BGR_EXT, GL_UNSIGNED_BYTE, 0/*Texture.Pixels*/);
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
#endif