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
        GlyphImages[i].Data = stbtt_GetCodepointBitmap(&FontInfo, ScaleFactor, ScaleFactor, Codepoint, &w, &h, &OffX, &OffY);
        GlyphImages[i].w    = w;
        GlyphImages[i].h    = h;
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

    render_piece Pieces[16];
    u32 UsedPieces;

    font Fonts[2];
} renderer;

internal void PushPiece(renderer *Renderer, render_piece Piece) {
    Renderer->Pieces[Renderer->UsedPieces] = Piece;
    Renderer->UsedPieces++;
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
    rv2   Advance = rv2_(0, 0);
    rv2   Offset  = rv2_(0, 0);
    rectf Rect;
    u32   Index = 0;
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
    for (u32 PieceIndex = 0; PieceIndex < Renderer->UsedPieces; PieceIndex++) {
        render_piece Piece = Renderer->Pieces[PieceIndex];

        if (Piece.Type == PIECE_RECT) {
            render_piece_rect Rect = Piece.Rect;
            glBegin(GL_POLYGON); {
                glColor4ub(Rect.Color.r, Rect.Color.g, Rect.Color.b, Rect.Color.a);
                
                glVertex2f(Rect.Rect.x,               Rect.Rect.y + Rect.Rect.h);
                glVertex2f(Rect.Rect.x + Rect.Rect.w, Rect.Rect.y + Rect.Rect.h);
                glVertex2f(Rect.Rect.x + Rect.Rect.w, Rect.Rect.y);
                glVertex2f(Rect.Rect.x,               Rect.Rect.y);
            } glEnd();
        }
        else
        if (Piece.Type == PIECE_GLYPH) {
            render_piece_glyph Glyph = Piece.Glyph;

            glBegin(GL_QUADS); {
                glColor4ub(Glyph.Color.r, Glyph.Color.g, Glyph.Color.b, Glyph.Color.a);

                rectf GlyphRect = Glyph.Font->GlyphRects[Glyph.Char - 32];
                
                glVertex2f(Glyph.Pos.x,               Glyph.Pos.y + GlyphRect.h);
                glVertex2f(Glyph.Pos.x + GlyphRect.w, Glyph.Pos.y + GlyphRect.h);
                glVertex2f(Glyph.Pos.x + GlyphRect.w, Glyph.Pos.y);
                glVertex2f(Glyph.Pos.x,               Glyph.Pos.y);
            } glEnd();
        }
    }
}

#endif//RENDER_GROUP_H