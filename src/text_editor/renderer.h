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

#include "lingo.h"
#include "maths.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"

typedef u16 id;

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
    id  Id;
    u32 NoChars;

    i32 Height;
    i32 Ascender;
    i32 Descender;
    i32 LineGap;

    i32  *GlyphAdvances;
    rv2  *GlyphOffsets;
    rect *GlyphRects;

    texture Atlas;
} font;

///////////////////////////////////////////////////////////

enum piece_type {
    PIECE_RECT,
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

    font Fonts[2];
    u32  UsedFonts;
} renderer;

///////////////////////////////////////////////////////////

internal void Clear(rv2 TargetDim, color Color) {
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

internal void Clip(rect ClipRect) {
    glScissor(ClipRect.x, ClipRect.y, ClipRect.w, ClipRect.x);
}

internal void RasterRect(rect Rect, color Color) {
    glBegin(GL_POLYGON); {
        glColor4f(Color.r, Color.g, Color.b, Color.a);
        glVertex2f(Rect.x,          Rect.y + Rect.h);
        glVertex2f(Rect.x + Rect.w, Rect.y + Rect.h);
        glVertex2f(Rect.x + Rect.w, Rect.y);
        glVertex2f(Rect.x,          Rect.y);
    } glEnd();
}

internal void RasterTextureRect(rv2 Pos, rect Rect, texture Texture, color Tint) {
    glBindTexture(GL_TEXTURE_2D, Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        rv2 TextureDim  = rv2_(Texture.w, Texture.h);

        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);

        glTexCoord2f(Rect.x / TextureDim.w, Rect.y / TextureDim.h);
        glVertex2f(Pos.x, Pos.y + Rect.h);

        glTexCoord2f((Rect.x + Rect.w) / TextureDim.w, Rect.y /TextureDim.h);
        glVertex2f(Pos.x + Rect.w, Pos.y + Rect.h);
        
        glTexCoord2f((Rect.x + Rect.w) / TextureDim.w, (Rect.y + Rect.h) / TextureDim.h);
        glVertex2f(Pos.x + Rect.w, Pos.y);

        glTexCoord2f(Rect.x / TextureDim.w, (Rect.y + Rect.h) / TextureDim.h);
        glVertex2f(Pos.x, Pos.y);
    } glEnd();
    glDisable(GL_TEXTURE_2D);
}

///////////////////////////////////////////////////////////

internal void PushPiece(renderer *Renderer, render_piece Piece) {
    Renderer->Pieces[Renderer->UsedPieces] = Piece;
    if (Renderer->UsedPieces < 1024)
        Renderer->UsedPieces++;
}

internal void Render(renderer *Renderer, iv2 TargetDim, colorb ClearColor) {
    Renderer->TargetClipRect.Dim = rv2_(TargetDim.x, TargetDim.y);
    Renderer->ClearColor         = ClearColor;

    Clear(Renderer->TargetClipRect.Dim, HexToColor(Renderer->ClearColor.rgba));

    for (u32 PieceIndex = 0; PieceIndex < Renderer->UsedPieces; PieceIndex++) {
        render_piece Piece = Renderer->Pieces[PieceIndex];
        rv2 Pos = Piece.Rect.Pos;
        rv2 Dim = Piece.Rect.Dim;

        if (Piece.Type == PIECE_RECT) {
            RasterRect(rect_(GetVecComps(Pos), GetVecComps(Dim)), HexToColor(Piece.Color.rgba));
        }
        else
        if (Piece.Type == PIECE_GLYPH) {
            render_piece_glyph Glyph = Piece.Glyph;
            RasterTextureRect(Pos, Renderer->Fonts[Glyph.FontId].GlyphRects[Glyph.Index],
                              Renderer->Fonts[Glyph.FontId].Atlas, HexToColor(Piece.Color.rgba));
        }
    }

    Renderer->UsedPieces = 0;
}

///////////////////////////////////////////////////////////

internal id LoadFont(renderer *Renderer, platform_api *p, c8 *Filename, u32 NoChars, r32 Size) {
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
        .GlyphRects    = p->AllocateMemory(NoChars * sizeof(rect)),
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
        Font.GlyphOffsets[i]  = rv2_(OffX, OffY + h);
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

        Font.GlyphRects[i] = rect_(OffsetX, OffsetY, GlyphImages[i].w, GlyphImages[i].h);
        OffsetX += (GlyphImages[i].w + 2 * Padding);

        if (OffsetX >= (Atlas.w - GlyphImages[i].w - Padding)) {
            OffsetX = Padding;
            OffsetY += (Size + 2 * Padding);
            if (OffsetY > (Atlas.h - Size - Padding))
                break;
        }

        if (Codepoint == ' ') {
            Font.GlyphRects[i].w = Font.GlyphAdvances[i];
            Font.GlyphRects[i].h = Font.Ascender;
        }
        if (Codepoint == '\t') {
            Font.GlyphRects[i].w = Font.GlyphAdvances[i];
            Font.GlyphRects[i].h = Font.Ascender;
        }
        if (Codepoint == '\r') {
            Font.GlyphRects[i].w = Font.GlyphAdvances[i];
            Font.GlyphRects[i].h = Font.Ascender;
        }
        if (Codepoint == '\n') {
            Font.GlyphRects[i].w = Font.GlyphAdvances[i];
            Font.GlyphRects[i].h = Font.Ascender;
        }
        
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

internal void DrawRect(renderer *Renderer, rect Rect, colorb Color) {
    render_piece Piece;

    Piece.Type     = PIECE_RECT;
    Piece.Rect.Pos = Rect.Pos;
    Piece.Rect.Dim = Rect.Dim;
    Piece.Color    = Color;

    if (AreRectsClipping(Renderer->TargetClipRect, Piece.Rect))
        PushPiece(Renderer, Piece);
}

internal void DrawGlyph(renderer *Renderer, id FontId, u32 Index, rv2 Pos, colorb Color) {
    render_piece Piece;

    Piece.Type         = PIECE_GLYPH;
    Piece.Rect.Pos     = Pos;
    Piece.Rect.Dim     = Renderer->Fonts[FontId].GlyphRects[Index].Dim;
    Piece.Color        = Color;
    Piece.Glyph.FontId = FontId;
    Piece.Glyph.Index  = Index;

    if (AreRectsClipping(Renderer->TargetClipRect, Piece.Rect)) {
        PushPiece(Renderer, Piece);
    }
}

typedef struct _render_text {
    c8 *Text;
    id  Font;
    rv2 Pos;
} render_text;

typedef enum _text_op {
    TEXT_OP_MEASURE,
    TEXT_OP_DRAW
} text_op;

//todo: clean this up
internal rect DoTextOp(text_op Op, renderer *Renderer, render_text *Text, colorb Color) {
    font *Font = &Renderer->Fonts[Text->Font];

    u32 Index;
    rv2 Offset;
    rv2 Pos = Text->Pos;

    rect Result = {0};
    Result.Pos = Pos;
    rect GlyphRect = {0};

    for (c8 *Char = Text->Text; *Char; Char++) {
        Index  = (*Char - 32 >= 0)? *Char - 32 : '?' - 32;
        Offset = Font->GlyphOffsets[Index];
        GlyphRect = rect_(Pos.x + Offset.x, Pos.y - Offset.y, GetVecComps(Font->GlyphRects[Index].Dim));
        if (*Char == ' ') {
            Pos.x += Font->GlyphAdvances[Index];// + Style->CharSpacing;
            continue;
        }
        if (Op == TEXT_OP_MEASURE) {
            Result = rect_Union(GlyphRect, Result);
        }
        else
        if (Op == TEXT_OP_DRAW) {
            DrawGlyph(Renderer, 0, Index, GlyphRect.Pos, Color);
        }
        Pos.x += Font->GlyphAdvances[Index];
    }

    
    Result.Pos = Text->Pos;
    Result.h  += Font->Descender; //times number of lines

    return Result;
}

internal rect MeasureText(renderer *Renderer, render_text *Text) {
    return DoTextOp(TEXT_OP_MEASURE, Renderer, Text, (colorb){0});
}

internal void DrawText(renderer *Renderer, render_text *Text, colorb Color) {
    DoTextOp(TEXT_OP_DRAW, Renderer, Text, Color);
}

//todo: pull some of these thing out to some "ui_context" struct or something
// internal u32 DrawMenu(renderer *Renderer, id Font, rv2 Pos, rv2 mPos, b32 Clicked) {
//     c8 *MenuItems[] = {
//         "item 1",
//         "item 2",
//         "item 3",
//         "item 4"
//     };

//     u32 ClickedIndex = -1;

//     for (u32 MenuItemIndex = 0; 
//              MenuItemIndex < ArrayCount(MenuItems);
//              MenuItemIndex++)
//     {
//         render_text Text = {
//             .Text = MenuItems[MenuItemIndex],
//             .Font = Font,
//             .Pos  = Pos
//         };

//         colorb Color = (colorb){0xFFFFFFFF};
//         rect MenuItemTextBounds = MeasureText(Renderer, &Text);
//         if (IsInsideRect(mPos, MenuItemTextBounds)) {
//             Color = (colorb){0xFFFF00FF};
//             if (Clicked) {
//                 Color = (colorb){0xFF0000FF};
//                 ClickedIndex = MenuItemIndex;
//             }
//         }

//         // DrawRect(Renderer, MenuItemTextBounds, (colorb){0x80808080});
//         DrawText(Renderer, &Text, Color);

//         Pos.y -= Renderer->Fonts[Font].LineGap;
//     }

//     return ClickedIndex;
// }
    
typedef struct _ui_ctx {
    rv2 mPos;
    b32 mLeftButtonIsDown;
    i16 dmWheel;

    id Hot;
    id Clicked;
    id Last;
    id Current;

    r32 Point;
} ui_ctx;

typedef struct _ui_style {
    id  Font;
    r32 CharSpacing;
    r32 LineSpacing;

    rv2 Padding;

    // colorb HotTextColor;
    // colorb ClickedTextColor;
    colorb DefaultTextColor;

    colorb HotButtonColor;
    colorb ClickedButtonColor;
    colorb DefaultButtonColor;

    r32 SliderHandleWidth;
    r32 SliderHandleHeight;
} ui_style;

internal b32 UiAddButton(renderer *Renderer, ui_ctx *Ctx, ui_style *Style, rv2 Pos, c8 *Str) {
    b32 WasClicked = 0;
    id  Me = Ctx->Current;

    render_text Text = {
        .Text = Str,
        .Font = Style->Font,
        .Pos  = Pos
    };

    Pos.x += Style->Padding.x;
    Pos.y += Style->Padding.y;

    colorb ButtonColor = Style->DefaultButtonColor;

    rect MenuItemTextBounds = MeasureText(Renderer, &Text);

    Text.Pos.x += Style->Padding.x/2;
    Text.Pos.y += Style->Padding.y/2;

    MenuItemTextBounds.w += Style->Padding.x;
    MenuItemTextBounds.h += Style->Padding.y;

    if (IsInsideRect(Ctx->mPos, MenuItemTextBounds)) {
        Ctx->Hot    = Me;
        ButtonColor = Style->HotButtonColor;
        if (Ctx->mLeftButtonIsDown) {
            Ctx->Clicked = Me;
            WasClicked   = 1;
            ButtonColor  = Style->ClickedButtonColor;
        }
    }

    DrawRect(Renderer, MenuItemTextBounds, ButtonColor);
    DrawText(Renderer, &Text, Style->DefaultTextColor);

    Ctx->Last = Me;
    Ctx->Current++; //todo: check for overflow.

    return WasClicked;
}

internal f32 UiAddSlider(renderer *Renderer, ui_ctx *Ctx, ui_style *Style, r32 LastValue, rv2 Pos, r32 Range) {
    r32 Value = LastValue*Range;
    id  Me    = Ctx->Current;

    rect SliderGroove = rect_(Pos.x, Pos.y, Range + Style->SliderHandleWidth, Style->SliderHandleWidth);
    rect SliderHandle = rect_(Pos.x + Value, Pos.y - Style->SliderHandleHeight/2 + Style->SliderHandleWidth/2,
                              Style->SliderHandleWidth, Style->SliderHandleHeight);

    colorb HandleColor = Style->DefaultButtonColor;

    if (IsInsideRect(Ctx->mPos, rect_Union(SliderGroove, SliderHandle))) {
        Ctx->Hot    = Me;
        HandleColor = Style->HotButtonColor;

        if (Ctx->dmWheel) {
            Value = Min(Value + ((f32)Ctx->dmWheel/Range), Range);
        }

        if (Ctx->mLeftButtonIsDown) {
            Ctx->Clicked = Me;
            HandleColor  = Style->ClickedButtonColor;
            Value = Min(Ctx->mPos.x - Pos.x, Range);
        }
    }

    if (Value < 0)
        Value = 0;

    DrawRect(Renderer, SliderGroove, (colorb){0x2A2A2AFF});
    DrawRect(Renderer, SliderHandle, HandleColor);

    Ctx->Last = Me;
    Ctx->Current++; //todo: check for overflow.

    return Value/Range;
}

// internal void DrawMenu(renderer *Renderer, ui_ctx *Ctx, ui_style *Style, rv2 Pos) {
//     c8 *MenuItems[] = {
//         "item 1",
//         "item 2",
//         "item 3",
//         "item 4"
//     };

//     u32 ClickedIndex = -1;

//     for (u32 MenuItemIndex = 0; 
//              MenuItemIndex < ArrayCount(MenuItems);
//              MenuItemIndex++)
//     {
//         render_text Text = {
//             .Text = MenuItems[MenuItemIndex],
//             .Font = Style->Font,
//             .Pos  = Pos
//         };

//         colorb Color = Style->DefaultTextColor;
//         rect   MenuItemTextBounds = MeasureText(Renderer, &Text);
//         if (IsInsideRect(Ctx->mPos, MenuItemTextBounds)) {
//             Ctx->Hot = MenuItemIndex;
//             Color    = Style->HotTextColor;
//             if (Ctx->mLeftButtonIsDown) {
//                 Ctx->Clicked = MenuItemIndex;
//                 Color        = Style->ClickedTextColor;
//             }
//         }

//         // DrawRect(Renderer, MenuItemTextBounds, (colorb){0x80808080});
//         DrawText(Renderer, &Text, Color);

//         Pos.y -= Renderer->Fonts[Style->Font].LineGap;
//     }
// }

#endif//RENDERER_H
