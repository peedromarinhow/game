#ifndef RENDERER_H
#define RENDERER_H

//todo:
//  0 - Make this a standalone renderer.
//
//  1 - Make the glyph's origins the origin of the origin
//      of the first glyph so that each glyph's  position
//      is relative to the text only? Too complex maybe?

//note:
//  heavily based on https://www.youtube.com/watch?v=ehVU2S-GXhM&

#include <windows.h>
#include <gl/gl.h>
#undef    DrawText //damn you, windows.h

#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"

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

///////////////////////////////////////////////////////////

enum piece_type {
    PIECE_RECT,
    PIECE_GLYPH
    //...
};

typedef struct _render_piece_head {
    rv2 Origin;
    u32 Type;
} render_piece_header;

typedef struct _render_piece_rect {
    rectf  Rect;
} render_piece_rect;

typedef struct _render_piece_glyph {
    u16 FontId;
    c8  Char;
} render_piece_glyph;

typedef struct _render_piece {
    u32    Type;
    rv2    Origin;
    rv2    Pos;
    colorb Color;
    union {
        render_piece_rect  Rect;
        render_piece_glyph Glyph;
    };
} render_piece;

typedef struct _renderer {
    rectf  TargetClipRect;
    colorb ClearColor;

    render_piece Pieces[1024];
    u32          UsedPieces;

    font Fonts[2];
    u32  UsedFonts;
} renderer;

///////////////////////////////////////////////////////////

internal u16 LoadFont(renderer *Renderer, platform_api *p, c8 *Filename, u32 NoChars, r32 Size) {
    file FontFile = p->LoadFile(Filename);
    if (!FontFile.Data)
         FontFile = p->LoadFile("c:/windows/fonts/arial.ttf");
    stbtt_fontinfo  FontInfo;
    stbtt_InitFont(&FontInfo, FontFile.Data, stbtt_GetFontOffsetForIndex(FontFile.Data, 0));
    p->FreeFile(FontFile);

    f32 ScaleFactor          = stbtt_ScaleForPixelHeight(&FontInfo, Size);
    f32 RequiredAreaForAtlas = 0;
    i32 Padding              = 2;

    i32 Ascender, Descender, LineGap;
    stbtt_GetFontVMetrics(&FontInfo, &Ascender, &Descender, &LineGap);

    NoChars = (NoChars > 0)? NoChars : 95;

    Ascender  *= ScaleFactor;
    Descender *= ScaleFactor;
    LineGap   *= ScaleFactor;

    LineGap = (LineGap == 0)? Ascender + -Descender : LineGap;

    font Font = {
        .Id      = Renderer->UsedFonts,
        .NoChars = NoChars,

        .Height    = (i32)(Ascender - Descender + LineGap),
        .Ascender  = (i32)(Ascender),
        .Descender = (i32)(Descender),
        .LineGap   = (i32)(LineGap),

        .GlyphAdvances = p->AllocateMemory(NoChars * sizeof(i32)),
        .GlyphOffsets  = p->AllocateMemory(NoChars * sizeof(rv2)),
        .GlyphRects    = p->AllocateMemory(NoChars * sizeof(rectf)),
    };

    i32 w, h, OffX, OffY, Advance;
    image *GlyphImages = p->AllocateMemory(NoChars * sizeof(image));
    for (u32 i = 0; i < NoChars; i++) {
        u32 Codepoint = i + 32;
        GlyphImages[i].Data = stbtt_GetCodepointBitmap(&FontInfo, ScaleFactor, ScaleFactor, Codepoint, &w, &h, &OffX, &OffY);
        GlyphImages[i].w    = w;
        GlyphImages[i].h    = h;
        stbtt_GetCodepointHMetrics(&FontInfo, Codepoint, &Advance, NULL);
        Font.GlyphAdvances[i] = Advance * ScaleFactor;
        Font.GlyphOffsets[i]  = rv2_(OffX, OffY);
        RequiredAreaForAtlas += (GlyphImages[i].w + 2 * Padding) *
                                (GlyphImages[i].h + 2 * Padding);
    }

    f32 GuessSize = Sqrt(RequiredAreaForAtlas) * 1.3f;
    i32 ImageSize = (i32)powf(2, ceilf(logf((f32)GuessSize)/logf(2)));
    image Atlas = {
        .w    = ImageSize,
        .h    = ImageSize,
        .Data = p->AllocateMemory(ImageSize * ImageSize * sizeof(u32))
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
        
        p->FreeMemory(GlyphImages[i].Data);
    }

    Font.Atlas.w  = Atlas.w;
    Font.Atlas.h  = Atlas.h;
    Font.Atlas.Id = 0;

    glGenTextures(1, &Font.Atlas.Id);
    glBindTexture(GL_TEXTURE_2D, Font.Atlas.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Font.Atlas.w, Font.Atlas.h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, Atlas.Data);
    
    p->FreeMemory(Atlas.Data);

    Renderer->Fonts[Renderer->UsedFonts] = Font;
    Renderer->UsedFonts++;

    return Renderer->Fonts[Renderer->UsedFonts - 1].Id;
}

///////////////////////////////////////////////////////////
internal void PushPiece(renderer *Renderer, render_piece Piece) {
    Renderer->Pieces[Renderer->UsedPieces] = Piece;
    Renderer->UsedPieces++;
}

internal void DrawRect(renderer *Renderer, rectf Rect, colorb Color) {
    render_piece Piece;

    Piece.Origin    = rv2_(0, 0);
    Piece.Type      = PIECE_RECT;
    Piece.Pos       = Rect.Pos;
    Piece.Rect.Rect = Rect;
    Piece.Color     = Color;

    if (AreRectsClipping(Renderer->TargetClipRect, Piece.Rect.Rect))
        PushPiece(Renderer, Piece);
}

internal void DrawGlyph(renderer *Renderer, u16 FontId, c8 Char, rv2 Pos, colorb Color) {
    render_piece Piece;

    Piece.Origin       = rv2_(0, 0);
    Piece.Type         = PIECE_GLYPH;
    Piece.Pos          = Pos;
    Piece.Color        = Color;
    Piece.Glyph.FontId = FontId;
    Piece.Glyph.Char   = Char;

    // if (AreRectsClipping(Renderer->TargetClipRect,
    //     rectf_(Piece.Pos.x, Piece.Pos.y,
    //            Renderer->Fonts[Piece.Glyph.FontId].GlyphRects[Char - 32].w,
    //            Renderer->Fonts[Piece.Glyph.FontId].GlyphRects[Char - 32].h)))
    // {
        PushPiece(Renderer, Piece);
    // }
}

internal void DrawText(renderer *Renderer, u16 FontId, rv2 Pos,
                       c8 *Text, r32 Size,
                       r32 LineSpacing, r32 CharSpacing,
                       colorb Color)
{
    font Font = Renderer->Fonts[FontId];

    rv2   Advance = rv2_(0, 0);
    rv2   Offset  = rv2_(0, 0);
    rectf Rect;
    u32   Index = 0;
    for (u32 i = 0; Text[i] != '\0'; i++) {
        Index    = Text[i] - 32;
        if (Text[i] == '\n') {
            Advance.y += Font.LineGap + LineSpacing;
            Advance.x  = 0;
        }
        else
        if (Text[i] == '\r') {
            Assert(1);
        }
        else {
            Offset = Font.GlyphOffsets[Index];
            Rect   = Font.GlyphRects[Index];
            DrawGlyph(Renderer, FontId, Text[i], rv2_(Pos.x + Offset.x + Advance.x, Pos.y - Offset.y - Advance.y - Rect.h), Color);
            Advance.x += Font.GlyphAdvances[Index] + CharSpacing;
        }
    }
}

void Clear(rv2 TargetDim, color Color) {
    glLoadIdentity();
    glViewport(0, 0, TargetDim.w, TargetDim.h);
    
    r32 a = 2.0f/TargetDim.w;
    r32 b = 2.0f/TargetDim.h;
    r32 Proj[] = {
        a, 0, 0, 0,
        0, b, 0, 0,
        0, 0, 1, 0,
       -1,-1, 0, 1
    };

    glLoadMatrixf(Proj);
    glClearColor(Color.r, Color.g, Color.b, Color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

internal void Render(renderer *Renderer, iv2 TargetDim, colorb ClearColor) {
    // Renderer->TargetClipRect.Dim = rv2_(TargetDim.x, TargetDim.y);
    Renderer->ClearColor         = ClearColor;

    Clear(Renderer->TargetClipRect.Dim, HexToColor(Renderer->ClearColor.rgba));

    for (u32 PieceIndex = 0; PieceIndex < Renderer->UsedPieces; PieceIndex++) {
        render_piece Piece = Renderer->Pieces[PieceIndex];
        rv2 Pos = Piece.Pos;

        Pos.x += Piece.Origin.x;
        Pos.y += Piece.Origin.y;

        if (Piece.Type == PIECE_RECT) {
            render_piece_rect Rect = Piece.Rect;
            glBegin(GL_POLYGON); {
                glColor4ub(Piece.Color.r, Piece.Color.g, Piece.Color.b, Piece.Color.a);
                
                glVertex2f(Pos.x,               Pos.y + Rect.Rect.h);
                glVertex2f(Pos.x + Rect.Rect.w, Pos.y + Rect.Rect.h);
                glVertex2f(Pos.x + Rect.Rect.w, Pos.y);
                glVertex2f(Pos.x,               Pos.y);
            } glEnd();
        }
        else
        if (Piece.Type == PIECE_GLYPH) {
            render_piece_glyph Glyph = Piece.Glyph;
            font Font                = Renderer->Fonts[Glyph.FontId];

            glBindTexture(GL_TEXTURE_2D, Font.Atlas.Id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glEnable(GL_TEXTURE_2D);
            glBegin(GL_QUADS); {
                rv2   AtlasDim  = rv2_(Font.Atlas.w, Font.Atlas.h);
                rectf GlyphRect = Font.GlyphRects[Glyph.Char - 32];

                glColor4ub(Piece.Color.r, Piece.Color.g, Piece.Color.b, Piece.Color.a);

                glTexCoord2f(GlyphRect.x /AtlasDim.w, GlyphRect.y /AtlasDim.h);
                glVertex2f(Pos.x, Pos.y + GlyphRect.h);

                glTexCoord2f((GlyphRect.x + GlyphRect.w)/AtlasDim.w, GlyphRect.y /AtlasDim.h);
                glVertex2f(Pos.x + GlyphRect.w, Pos.y + GlyphRect.h);
                
                glTexCoord2f((GlyphRect.x + GlyphRect.w)/AtlasDim.w, (GlyphRect.y + GlyphRect.h)/AtlasDim.h);
                glVertex2f(Pos.x + GlyphRect.w, Pos.y);

                glTexCoord2f(GlyphRect.x /AtlasDim.w, (GlyphRect.y + GlyphRect.h)/AtlasDim.h);
                glVertex2f(Pos.x, Pos.y);
            } glEnd();
            glDisable(GL_TEXTURE_2D);
        }
    }

    Renderer->UsedPieces = 0;
}

#endif//RENDERER_H