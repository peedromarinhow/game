#ifndef RENDER_GROUP_H
#define RENDER_GROUP_H

//note:
//  heavily based on https://www.youtube.com/watch?v=ehVU2S-GXhM&

//note:
//  These are the vertices of the quad of the character
//      Rect = Font->GlyphRects[Index];
//      Rect.x          /Dim.w,  Rect.y          /Dim.h);
//     (Rect.x + Rect.w)/Dim.w,  Rect.y          /Dim.h);
//     (Rect.x + Rect.w)/Dim.w, (Rect.y + Rect.h)/Dim.h);
//      Rect.x          /Dim.w, (Rect.y + Rect.h)/Dim.h);

#include <windows.h>
#include <gl/gl.h>
#undef DrawText

#include "lingo.h"
#include "maths.h"

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

#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"

typedef struct _font {
    u16 Id;
    u32 NoChars;

    i32 Height;
    i32 Ascender;
    i32 Descender;
    i32 LineGap;

    i32   *GlyphAdvances;
    rv2   *GlyphOffsets;
    rectf *GlyphRects;

    texture Atlas;
} font;

internal font LoadFont(platform_api p, c8 *Filename, u32 NoChars, r32 Size) {
    file FontFile = p.LoadFile(Filename);
    if (!FontFile.Data)
         FontFile = p.LoadFile("c:/windows/fonts/arial.ttf");
    stbtt_fontinfo  FontInfo;
    stbtt_InitFont(&FontInfo, FontFile.Data, stbtt_GetFontOffsetForIndex(FontFile.Data, 0));
    p.FreeFile(FontFile);

    i32 w, h, OffX, OffY, Advance;
    f32 ScaleFactor          = stbtt_ScaleForPixelHeight(&FontInfo, Size);
    f32 RequiredAreaForAtlas = 0;
    i32 Padding              = 2;

    i32 Ascender, Descender, LineGap;
    stbtt_GetFontVMetrics(&FontInfo, &Ascender, &Descender, &LineGap);

    NoChars = (NoChars > 0)? NoChars : 95;

    Ascender  *= ScaleFactor;
    Descender *= ScaleFactor;
    LineGap   *= ScaleFactor;

    font Font = {
        .Id      = 0,
        .NoChars = NoChars,

        .Height    = (i32)(Ascender - Descender + LineGap),
        .Ascender  = (i32)(Ascender),
        .Descender = (i32)(Descender),
        .LineGap   = (i32)(LineGap),

        .GlyphAdvances = p.AllocateMemory(NoChars * sizeof(i32)),
        .GlyphOffsets  = p.AllocateMemory(NoChars * sizeof(rv2)),
        .GlyphRects    = p.AllocateMemory(NoChars * sizeof(rectf)),
    };

    image *GlyphImages = p.AllocateMemory(NoChars * sizeof(image));
    for (u32 i = 0; i < NoChars; i++) {
        u32 Codepoint = i + 32;
        GlyphImages[i].Data   = stbtt_GetCodepointBitmap(&FontInfo, ScaleFactor, ScaleFactor, Codepoint, &w, &h, &OffX, &OffY);
        stbtt_GetCodepointHMetrics(&FontInfo, Codepoint, &Advance, NULL);
        Font.GlyphAdvances[i] = Advance;
        Font.GlyphOffsets[i]  = rv2_(OffX, OffY);
        RequiredAreaForAtlas += (GlyphImages[i].w + 2 * Padding) *
                                (GlyphImages[i].h + 2 * Padding);
    }

    f32 GuessSize = Sqrt(RequiredAreaForAtlas) * 1.3f;
    i32 ImageSize = (i32)powf(2, ceilf(logf((f32)GuessSize)/logf(2)));
    image Atlas = {
        .w    = ImageSize,
        .h    = ImageSize,
        .Data = p.AllocateMemory(ImageSize * ImageSize * sizeof(u32))
    };

    i32 OffsetX = Padding;
    i32 OffsetY = Padding;

    for (u32 i = 0; i < NoChars; i++) {
        u32 Codepoint = i + 32;
        // Copy pixel data from fc.data to atlas
        for (i32 y = 0; y < GlyphImages[i].h; y++) {
            for (i32 x = 0; x < GlyphImages[i].w; x++) {
                ((u32 *)Atlas.Data)[(OffsetY + y)*Atlas.w + (OffsetX + x)] =
                    ((((u8 *)GlyphImages[i].Data)[y*GlyphImages[i].w + x]) << 24) |
                    ((((u8 *)GlyphImages[i].Data)[y*GlyphImages[i].w + x]) << 16) |
                    ((((u8 *)GlyphImages[i].Data)[y*GlyphImages[i].w + x]) <<  8) |
                    ((((u8 *)GlyphImages[i].Data)[y*GlyphImages[i].w + x]) <<  0);
            }
        }

        Font.GlyphRects[i] = rectf_(OffsetX, OffsetY, GlyphImages[i].w, GlyphImages[i].h);
        OffsetX += (GlyphImages[i].w + 2 * Padding);

        if (OffsetX >= (Atlas.w - GlyphImages[i].w - Padding)) {
            OffsetX = Padding;
            OffsetY += (Size + 2 * Padding);
            if (OffsetY > (Atlas.h - Size - Padding))
                break;
        }

        if (Codepoint == ' ')
            Font.GlyphRects[i].w = Font.GlyphAdvances[i];
        if (Codepoint == '\t')
            Font.GlyphRects[i].w = Font.GlyphAdvances[i];
        if (Codepoint == '\r')
            Font.GlyphRects[i].w = Font.GlyphAdvances[i];
        if (Codepoint == '\n')
            Font.GlyphRects[i].w = Font.GlyphAdvances[i];
        
        p.FreeMemory(GlyphImages[i].Data);
    }


    Font.Atlas.w  = Atlas.w;
    Font.Atlas.h  = Atlas.h;
    Font.Atlas.Id = 0;

    glGenTextures(1, &Font.Atlas.Id);
    glBindTexture(GL_TEXTURE_2D, Font.Atlas.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Font.Atlas.w, Font.Atlas.h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, Atlas.Data);
    
    p.FreeMemory(Atlas.Data);

    return Font;
}

typedef struct _render_piece_head {
    u32 Type;
    u32 Temp;
} render_piece_header;

typedef struct _render_piece_rect {
    rectf  Rect;
    bcolor Color;
} render_piece_rect;

typedef struct _render_piece_glyph {
    rv2    Pos;
    bcolor Color;

    font *Font; //todo: use a u16 Id.
    c8    Char;
} render_piece_glyph;

enum piece_type {
    PIECE_RECT,
    PIECE_GLYPH
    //...
};

typedef struct _render_piece {
    u32 Type;
    u32 Temp;
    union {
        render_piece_rect  Rect;
        render_piece_glyph Glyph;
    };
} render_piece;

typedef struct _renderer {
    iv2    TargetDim;
    bcolor ClearColor;

    void *PushBuffer;
    u32 BufferSize;

    font Fonts[2];
} renderer;

internal void PushPiece(renderer *Renderer, render_piece Piece) {
    void *Base = Renderer->PushBuffer;
   *(u32 *)(Renderer->PushBuffer) = Piece.Type;
    Renderer->PushBuffer = (u8 *)((u32 *)Renderer->PushBuffer + 1);
    
    if (Piece.Type == PIECE_RECT) {
      *(render_piece_rect *)Renderer->PushBuffer = Piece.Rect;
       Renderer->PushBuffer = (u8 *)Renderer->PushBuffer + sizeof(render_piece_rect);
    }
    else
    if (Piece.Type == PIECE_GLYPH) {
      *(render_piece_glyph *)Renderer->PushBuffer = Piece.Glyph;
       Renderer->PushBuffer = (u8 *)((render_piece_glyph *)Renderer->PushBuffer + 1);
    }

    Renderer->PushBuffer = Base;
}

internal void DrawRect(renderer *Renderer, rectf Rect, bcolor Color) {
    render_piece Piece;

    Piece.Type       = PIECE_RECT;
    Piece.Temp       = 63;
    Piece.Rect.Rect  = Rect;
    Piece.Rect.Color = Color;

    PushPiece(Renderer, Piece);
}

internal void DrawGlyph(renderer *Renderer, font *Font, c8 Char, rv2 Pos, bcolor Color) {
    render_piece Piece;

    Piece.Type        = PIECE_GLYPH;
    Piece.Glyph.Pos   = Pos;
    Piece.Glyph.Color = Color;
    Piece.Glyph.Font  = Font;
    Piece.Glyph.Char  = Char;

    PushPiece(Renderer, Piece);
}

internal void DrawText(renderer *Renderer, font *Font, rv2 Pos,
                       c8 *Text, r32 Size,
                       r32 LineSpacing, r32 CharSpacing,
                       bcolor Color)
{
    u32 Index       = 0;

    rv2   Advance;
    rv2   Offset;
    rectf Rect;
    for (u32 i = 0; Text[i] != '\0'; i++) {
        Index    = Text[i] - 32;
        if (Text[i] == '\n') {
            Advance.y += Font->LineGap + LineSpacing;
            Advance.x  = 0;
        }
        Offset = Font->GlyphOffsets[Index];
        Rect   = Font->GlyphRects[Index];
        DrawGlyph(Renderer, Font, Text[i], rv2_(Pos.x + Offset.x + Advance.x, Pos.y - Offset.y - Advance.y - Rect.h), Color);
        Advance.x += Font->GlyphAdvances[Index] + CharSpacing;
    }
}

#define CastStructAndIncrementIt(type, Data) (type *)Data; Data += sizeof(type)

internal void Render(renderer *Renderer) {
    glLoadIdentity();
    glViewport(0, 0, Renderer->TargetDim.w, Renderer->TargetDim.h);
    
    r32 a = 2.0f/Renderer->TargetDim.w;
    r32 b = 2.0f/Renderer->TargetDim.h;
    r32 Proj[] = {
        a, 0, 0, 0,
        0, b, 0, 0,
        0, 0, 1, 0,
       -1,-1, 0, 1
    };

    glLoadMatrixf(Proj);
    glClearColor(Renderer->ClearColor.r, Renderer->ClearColor.g, Renderer->ClearColor.b, Renderer->ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    u8 *BufferBase = Renderer->PushBuffer;
    u8 *BufferEnd  = (u8 *)Renderer->PushBuffer + Renderer->BufferSize;

    while (BufferBase < BufferEnd) {
        render_piece_header *Header = CastStructAndIncrementIt(render_piece_header, BufferBase);
        
        u32 Type = Header->Type;
        if (Type == PIECE_RECT) {
            render_piece_rect *Rect = CastStructAndIncrementIt(render_piece_rect, BufferBase);

            rectf RectData = Rect->Rect;

            glColor4b(Rect->Color.r, Rect->Color.g, Rect->Color.b, Rect->Color.a);
            glBegin(GL_QUADS); {
                glVertex2f(RectData.x,              RectData.y);
                glVertex2f(RectData.x + RectData.w, RectData.y);
                glVertex2f(RectData.x + RectData.w, RectData.y + RectData.h);
                glVertex2f(RectData.x,              RectData.y + RectData.h);
            } glEnd();
        }
        else
        if (Type == PIECE_GLYPH) {
            render_piece_glyph *Glyph = CastStructAndIncrementIt(render_piece_glyph, BufferBase);
        }
    }
}

#endif//RENDER_GROUP_H