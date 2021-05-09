#ifndef RENDERER_H
///////////////////////////////////////////////////////////
// RENDERER
#define RENDERER_H
//todo:
//  0 - Make this a standalone renderer.
//
//  1 - Make the glyph's origins the origin of the origin
//      of the first glyph so that each glyph's  position
//      is relative to the text only? Too complex maybe?

//note:
//  heavily based on https://www.youtube.com/watch?v=ehVU2S-GXhM&

#define GL_SCISSOR_TEST 0x0C11
#define GL_BLEND        0x0BE2

#pragma comment(lib, "freetype")

#include "libs/ft2build.h"
#include FT_FREETYPE_H

#include "lingo.h"
#include "maths.h"
#include "platform.h"

typedef struct _font {
    u32 NoChars;
    i32 Height;
    i32 Ascender;
    i32 Descender;
    i32 LineGap;
    r32  *Advances;
    rv2  *Bearings;
    rect *Rects;
    texture Atlas;
} font;

///////////////////////////////////////////////////////////

enum piece_type {
    PIECE_RECT,
    PIECE_CLIP,
    PIECE_UNCLIP,
    PIECE_GLYPH
    //...
};

typedef struct _render_piece_glyph {
    id FontId;
    id Index;
} render_piece_glyph;

typedef struct _render_piece {
    u32    Type;
    rect   Rect;
    colorb Color;
    union {
        render_piece_glyph Glyph;
    };
} render_piece;

typedef struct _renderer {
    rect  TargetClipRect;
    colorb ClearColor;

    render_piece Pieces[1024];
    u32          UsedPieces;

    rect Clips[8];
    u32  UsedClips;

    font Fonts[3];
    u32  UsedFonts;
} renderer;

///////////////////////////////////////////////////////////

// internal void DEBUG_DrawFontAtlas(texture Texture) {
//     rect Rect = rect_(0, 0, Texture.w, Texture.h);
//     rv2 Pos = rv2_(0, 0);
//     glBindTexture(GL_TEXTURE_2D, Texture.Id);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glEnable(GL_TEXTURE_2D);
//     glBegin(GL_QUADS); {
//         rv2 TextureDim  = rv2_(Texture.w, Texture.h);

//         glTexCoord2f(Rect.x / TextureDim.w, Rect.y / TextureDim.h);
//         glVertex2f(Pos.x, Pos.y + Rect.h);

//         glTexCoord2f((Rect.x + Rect.w) / TextureDim.w, Rect.y /TextureDim.h);
//         glVertex2f(Pos.x + Rect.w, Pos.y + Rect.h);
        
//         glTexCoord2f((Rect.x + Rect.w) / TextureDim.w, (Rect.y + Rect.h) / TextureDim.h);
//         glVertex2f(Pos.x + Rect.w, Pos.y);

//         glTexCoord2f(Rect.x / TextureDim.w, (Rect.y + Rect.h) / TextureDim.h);
//         glVertex2f(Pos.x, Pos.y);
//     } glEnd();
//     glDisable(GL_TEXTURE_2D);
// }

///////////////////////////////////////////////////////////

internal void PushPiece(renderer *Renderer, render_piece Piece) {
    Renderer->Pieces[Renderer->UsedPieces] = Piece;
    if (Renderer->UsedPieces < 1024)
        Renderer->UsedPieces++;
}

internal void Render(platform_graphics_api *gApi, renderer *Renderer, iv2 TargetDim, colorb ClearColor) {
    Renderer->TargetClipRect.Dim = rv2_(TargetDim.x, TargetDim.y);
    Renderer->ClearColor         = ClearColor;

    rect ClipRect = Renderer->TargetClipRect;
    gApi->Clear(Renderer->TargetClipRect.Dim, HexToColor(Renderer->ClearColor.rgba));

    for (u32 PieceIndex = 0; PieceIndex < Renderer->UsedPieces; PieceIndex++) {
        render_piece Piece = Renderer->Pieces[PieceIndex];
        rv2 Pos = Piece.Rect.Pos;
        rv2 Dim = Piece.Rect.Dim;

        gApi->Clip(ClipRect);

        if (Piece.Type == PIECE_RECT) {
            gApi->RasterRect(rect_(GetVecComps(Pos), GetVecComps(Dim)), HexToColor(Piece.Color.rgba));
        }
        else
        if (Piece.Type == PIECE_CLIP) {
            gApi->Enable(GL_SCISSOR_TEST);
            ClipRect = Piece.Rect;
        }
        else
        if (Piece.Type == PIECE_UNCLIP) {
            gApi->Disable(GL_SCISSOR_TEST);
            ClipRect = Renderer->TargetClipRect;
        }
        else
        if (Piece.Type == PIECE_GLYPH) {
            render_piece_glyph Glyph = Piece.Glyph;
            gApi->RasterTextureRect(Pos, Renderer->Fonts[Glyph.FontId].Rects[Glyph.Index],
                                    Renderer->Fonts[Glyph.FontId].Atlas, HexToColor(Piece.Color.rgba));
        }
    }
    Renderer->UsedPieces = 0;
}

///////////////////////////////////////////////////////////

internal font LoadFont(platform_graphics_api *g, platform_api *p, FT_Library *FreeTypeLib, c8 *Filename, i32 Height) {
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

    g->GenAndBindAndLoadTexture(&AtlasImage, &Font.Atlas);
    
    FT_Done_Face(Face);
    
    return Font;
}

///////////////////////////////////////////////////////////

internal void DrawRect(renderer *Renderer, rect Rect, colorb Color) {
    render_piece Piece;

    Piece.Type     = PIECE_RECT;
    Piece.Rect.Pos = Rect.Pos;
    Piece.Rect.Dim = Rect.Dim;
    Piece.Color    = Color;

    if (AreRectsClipping(Renderer->TargetClipRect, Piece.Rect))
        PushPiece(Renderer, Piece);
}

internal void DrawPushClip(renderer *Renderer, rect Clip) {
    render_piece Piece;

    Piece.Type     = PIECE_CLIP;
    Piece.Rect.Pos = Clip.Pos;
    Piece.Rect.Dim = Clip.Dim;

    if (AreRectsClipping(Renderer->TargetClipRect, Piece.Rect))
        PushPiece(Renderer, Piece);
}

internal void DrawPopClip(renderer *Renderer) {
    render_piece Piece = {0};

    Piece.Type = PIECE_UNCLIP;

    if (AreRectsClipping(Renderer->TargetClipRect, Piece.Rect))
        PushPiece(Renderer, Piece);
}

internal void DrawGlyph(renderer *Renderer, id FontId, u32 Index, rv2 Pos, colorb Color) {
    render_piece Piece;

    Piece.Type         = PIECE_GLYPH;
    Piece.Rect.Pos     = Pos;
    Piece.Rect.Dim     = Renderer->Fonts[FontId].Rects[Index].Dim;
    Piece.Color        = Color;
    Piece.Glyph.FontId = FontId;
    Piece.Glyph.Index  = Index;

    if (AreRectsClipping(Renderer->TargetClipRect, Piece.Rect)) {
        PushPiece(Renderer, Piece);
    }
}

typedef enum _text_op {
    TEXT_OP_MEASURE,
    TEXT_OP_DRAW
} text_op;

internal rv2 DoTextOp(text_op Op, renderer *Renderer, c8 *Text, id FontId, rv2 Pos, colorb Color) {
    rv2 Result = {-100000, -100000};

    font *Font = &Renderer->Fonts[FontId];

    for (c8 *Char = Text; *Char; Char++) {
        u32 Index = (*Char - 32 >= 0)? *Char - 32 : '?' - 32;

        r32  Advance = Font->Advances[Index];
        rv2  Bearing = Font->Bearings[Index];
        rect Rect    = Font->Rects[Index];

        rv2 GlyphPos = rv2_(Pos.x + Bearing.x, Pos.y - (Rect.h - Bearing.y));

        if (Op == TEXT_OP_MEASURE) {
            Result = rect_Union(rect_(GlyphPos.x, GlyphPos.y, Rect.w, Rect.h),
                                rect_(0, 0, Result.w, Result.h)).Dim;
        }
        else
        if (Op == TEXT_OP_DRAW) {
            DrawGlyph(Renderer, 0, Index, GlyphPos, Color);
        }

        Pos.x += (i32)Advance >> 6; //note: what the fuck?
    }

    return Result;
}

internal rv2 MeasureText(renderer *Renderer, c8 *Text, id FontId) {
    return DoTextOp(TEXT_OP_MEASURE, Renderer, Text, FontId, rv2_(0, 0), (colorb){0});
}

internal void DrawText(renderer *Renderer, c8 *Text, id FontId, rv2 Pos, colorb Color) {
    DoTextOp(TEXT_OP_DRAW, Renderer, Text, FontId, Pos, Color);
}
#endif//RENDERER_H