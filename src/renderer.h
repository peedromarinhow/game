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
    u32  *Codepoints;
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

/////////////////////////////////////////////////////////

#include "windows.h"
#include "gl/gl.h"

#undef DrawText
#undef OpenFile

internal void DEBUG_DrawFontAtlas(texture Texture) {
    rect Rect = rect_(0, 0, Texture.w, Texture.h);
    rv2 Pos = rv2_(400, -400);
    glBindTexture(GL_TEXTURE_2D, Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        rv2 TextureDim  = rv2_(Texture.w, Texture.h);

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

/////////////////////////////////////////////////////////

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

//todo:
//  1 - reduce memory usabe by not loading unecessary glyphs

internal font LoadFont(platform_graphics_api *gApi, platform_api *Api, memory_arena *Arena, c8 *Filename, i32 Height) {
    FT_Library FreeTypeLib;
    FT_Init_FreeType(&FreeTypeLib);

    FT_Face Face = {0};
    font    Font = {0};
    if (FT_New_Face(FreeTypeLib, Filename, 0, &Face))
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
    Font.Codepoints = PushToArena(Arena, NoChars * sizeof(u32));//Api->AllocateMemory(NoChars * sizeof(u32));
    Font.Advances   = PushToArena(Arena, NoChars * sizeof(rv2));//Api->AllocateMemory(NoChars * sizeof(rv2));
    Font.Bearings   = PushToArena(Arena, NoChars * sizeof(rv2));//Api->AllocateMemory(NoChars * sizeof(rv2));
    Font.Rects      = PushToArena(Arena, NoChars * sizeof(rect));//Api->AllocateMemory(NoChars * sizeof(rect));

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
        // .Data = Api->AllocateMemory(ImageSize * ImageSize)
        .Data = Api->AllocateMemory(ImageSize * ImageSize * sizeof(u32))
    };

    i32 OffsetX = Padding;
    i32 OffsetY = Padding;

    for (u32 Char = 0; Char < NoChars; Char++) {
        u32 Index = Char + 32;//FT_Get_Char_Index(Face, Char);
        if (FT_Get_Char_Index(Face, Index) == 0)
            continue;
        if (FT_Load_Char(Face, Index, FT_LOAD_RENDER))
            continue;

        i32 w = Face->glyph->bitmap.width;
        i32 h = Face->glyph->bitmap.rows;

        for (i32 y = 0; y < h; y++) {
            for (i32 x = 0; x < w; x++) {
                // ((u8 *)AtlasImage.Data)[(OffsetY + y) * AtlasImage.w + (OffsetX + x)] =
                //     (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]);
                ((u32 *)AtlasImage.Data)[(OffsetY + y) * AtlasImage.w + (OffsetX + x)] =
                    ( (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]) << 24 ) |
                    ( (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]) << 16 ) |
                    ( (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]) << 8  ) |
                    ( (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]) << 0  );
            }
        }

        Font.Codepoints[Index] = Index;
        Font.Advances[Index] = Face->glyph->advance.x >> 6;
        Font.Bearings[Index] = rv2_(Face->glyph->bitmap_left, Face->glyph->bitmap_top);
        Font.Rects[Index] = rect_(OffsetX, OffsetY, w, h);
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

    gApi->GenAndBindAndLoadTexture(&AtlasImage, &Font.Atlas);
    
    FT_Done_Face(Face);
    FT_Done_FreeType(FreeTypeLib);
    
    return Font;
}

/*internal font LoadIconsFont(platform_graphics_api *gApi, platform_api *Api, c8 *Filename) {
    FT_Library FreeTypeLib;
    FT_Init_FreeType(&FreeTypeLib);

    FT_Face Face = {0};
    font    Font = {0};
    if (FT_New_Face(FreeTypeLib, Filename, 0, &Face))
        return Font;

    i32 Height  = 16;
    u32 NoChars = Face->num_glyphs;
    i32 Padding = 2;
    f32 RequiredAreaForAtlas = 0;

    // for (i32 Char = 0; Char < Face->num_glyphs; Char++) {
    //     if (FT_Get_Char_Index(Face, Char + 0xE000) == 0)
    //         continue;
    //     NoChars++;
    // }
    
    FT_Set_Pixel_Sizes(Face, 0, Height);

    RequiredAreaForAtlas = NoChars * Height*Height;
    Font.NoChars = NoChars*2;
    Font.Height    = Height;
    Font.Ascender  = Face->ascender;
    Font.Descender = Face->descender;
    Font.LineGap   = 0;
    Font.Codepoints = Api->AllocateMemory(NoChars * sizeof(u32));
    Font.Advances   = Api->AllocateMemory(NoChars * sizeof(rv2));
    Font.Bearings   = Api->AllocateMemory(NoChars * sizeof(rv2));
    Font.Rects      = Api->AllocateMemory(NoChars * sizeof(rect));

    f32 GuessSize = Sqrt(RequiredAreaForAtlas);
    i32 ImageSize = (i32)powf(2, ceilf(logf((f32)GuessSize)/logf(2)));
    image AtlasImage = {
        .w    = ImageSize,
        .h    = ImageSize,
        // .Data = Api->AllocateMemory(ImageSize * ImageSize)
        .Data = Api->AllocateMemory(ImageSize * ImageSize * sizeof(u32))
    };

    i32 OffsetX = Padding;
    i32 OffsetY = Padding;

    for (u32 Char = 0; Char < NoChars; Char++) {
        u32 Index = Char + 0xE000;
        if (FT_Get_Char_Index(Face, Index) == 0)
            continue;
        if (FT_Load_Char(Face, Index, FT_LOAD_RENDER))
            continue;

        i32 w = Face->glyph->bitmap.width;
        i32 h = Face->glyph->bitmap.rows;

        for (i32 y = 0; y < h; y++) {
            for (i32 x = 0; x < w; x++) {
                // ((u8 *)AtlasImage.Data)[(OffsetY + y) * AtlasImage.w + (OffsetX + x)] =
                //     (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]);
                ((u32 *)AtlasImage.Data)[(OffsetY + y) * AtlasImage.w + (OffsetX + x)] =
                    ( (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]) << 24 ) |
                    ( (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]) << 16 ) |
                    ( (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]) << 8  ) |
                    ( (((u8 *)Face->glyph->bitmap.buffer)[y * w + x]) << 0  );
            }
        }

        Font.Codepoints[Char] = Char;
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

    gApi->GenAndBindAndLoadTexture(&AtlasImage, &Font.Atlas);
    
    FT_Done_Face(Face);
    FT_Done_FreeType(FreeTypeLib);
    
    return Font;
}*/

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

//note: stolen from raylib
int GetNextCodepoint(const c8 *text, u32 *bytesProcessed)
{
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

internal u32 GetGlyphIndex(font *Font, u32 Codepoint) {
    u32 Index = '?';

    for (u32 i = 0; i < Font->NoChars; i++) {
        if (Font->Codepoints[i] == Codepoint) {
            Index = i;
            break;
        }
    }

    return Index;
}

internal rv2 DoTextOp(text_op Op, renderer *Renderer, c8 *Text, id FontId, rv2 Pos, colorb Color, r32 Height) {
    rv2   Result = {-100000, -100000};
    font *Font = &Renderer->Fonts[FontId];
    r32   ScaleFactor = Height/Font->Height;

    for (c8 *Char = Text; *Char; Char++) {
        u32 NoCodepointBytes = 0;
        u32 Codepoint = GetNextCodepoint(Char, &NoCodepointBytes);
        u32 Index = GetGlyphIndex(Font, Codepoint);//(*Char - 32 >= 0)? *Char - 32 : '?' - 32;
        if (Codepoint == 0x3f) NoCodepointBytes = 1;

        r32 Advance = Font->Advances[Index];
        rv2 Bearing = rv2_(Font->Bearings[Index].x * ScaleFactor,
                           Font->Bearings[Index].y * ScaleFactor);
        rect Rect = rect_(Font->Rects[Index].x * ScaleFactor, Font->Rects[Index].y * ScaleFactor,
                          Font->Rects[Index].w * ScaleFactor, Font->Rects[Index].h * ScaleFactor);

        rv2 GlyphPos = rv2_(Pos.x + Bearing.x, Pos.y - (Rect.h - Bearing.y));

        if (Op == TEXT_OP_MEASURE) {
            Result = rect_Union(rect_(GlyphPos.x, GlyphPos.y, Rect.w, Rect.h),
                                rect_(0, 0, Result.w, Result.h)).Dim;
        }
        else
        if (Op == TEXT_OP_DRAW) {
            DrawGlyph(Renderer, FontId, Index, GlyphPos, Color/*, ScaleFactor*/);
        }

        Pos.x += (i32)Advance * ScaleFactor;

        Char += (NoCodepointBytes - 1);
    }

    return Result;
}

internal rv2 MeasureText(renderer *Renderer, c8 *Text, id FontId) {
    return DoTextOp(TEXT_OP_MEASURE, Renderer, Text, FontId, rv2_(0, 0), (colorb){0}, 16);
}

internal void DrawText(renderer *Renderer, c8 *Text, id FontId, rv2 Pos, colorb Color) {
    DoTextOp(TEXT_OP_DRAW, Renderer, Text, FontId, Pos, Color, 16);
}
#endif//RENDERER_H