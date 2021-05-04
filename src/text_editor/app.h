#ifndef APP_H
#define APP_H

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

#include <windows.h>
#include <gl/gl.h>
#undef DrawText //damn you, windows.h
#undef OpenFile

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
    glScissor(ClipRect.x, ClipRect.y, ClipRect.w, ClipRect.h);
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

    rect ClipRect = Renderer->TargetClipRect;
    Clear(Renderer->TargetClipRect.Dim, HexToColor(Renderer->ClearColor.rgba));

    for (u32 PieceIndex = 0; PieceIndex < Renderer->UsedPieces; PieceIndex++) {
        render_piece Piece = Renderer->Pieces[PieceIndex];
        rv2 Pos = Piece.Rect.Pos;
        rv2 Dim = Piece.Rect.Dim;

        Clip(ClipRect);

        if (Piece.Type == PIECE_RECT) {
            RasterRect(rect_(GetVecComps(Pos), GetVecComps(Dim)), HexToColor(Piece.Color.rgba));
        }
        else
        if (Piece.Type == PIECE_CLIP) {
            glEnable(GL_SCISSOR_TEST);
            ClipRect = Piece.Rect;
        }
        else
        if (Piece.Type == PIECE_UNCLIP) {
            glDisable(GL_SCISSOR_TEST);
            ClipRect = Renderer->TargetClipRect;
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

        .Height    = (i32)(Ascender + Descender),
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
    Piece.Rect.Dim     = Renderer->Fonts[FontId].GlyphRects[Index].Dim;
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

internal rect DoTextOp(text_op Op, renderer *Renderer, c8 *Text, id FontId, rv2 Pos, colorb Color) {
    font *Font = &Renderer->Fonts[FontId];

    u32 Index;
    rv2 Offset;

    rect Result = {0};
    Result.Pos = Pos;
    rect GlyphRect = {0};

    for (c8 *Char = Text; *Char; Char++) {
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

    
    Result.Pos = Pos;
    Result.h  += Font->Descender; //times number of lines

    return Result;
}

internal rect MeasureText(renderer *Renderer, c8 *Text, id Font, rv2 Pos) {
    return DoTextOp(TEXT_OP_MEASURE, Renderer, Text, Font, Pos, (colorb){0});
}

internal void DrawText(renderer *Renderer, c8 *Text, id Font, rv2 Pos, colorb Color) {
    DoTextOp(TEXT_OP_DRAW, Renderer, Text, Font, Pos, Color);
}
#endif//RENDERER_H



#ifndef UI_H
///////////////////////////////////////////////////////////
// UI
#define UI_H

#include <stdio.h>
#include "lingo.h"

typedef struct _ui_ctx {
    id  Hot;
    id  Clicked;
    id  Last;
    id  Current;
    u32 NoItems;
} ui_ctx;

typedef struct _ui_style {
    id MainFont;
    id MonoFont;
    id IconFont;
    // rv2 TextSpacing;

    colorb BackColor;
    colorb HotColor;
    colorb ClickedColor;
    colorb DefaultColor;
    
    rv2 Padding;
} ui_style;

typedef struct _ui_input {
    rv2 mPos;
    b32 mLeft;
    b32 mRight;
    i16 mWheel;
} ui_input;

internal b32 uiButton(ui_ctx *Ctx, ui_style *Style, ui_input *Input, renderer *Renderer, rv2 Pos, c8 *Text) {
    b32 WasClicked = 0;
    id  Me = Ctx->Current;

    rv2 TextPos = {
        .x = Pos.x + Style->Padding.x,
        .y = Pos.y + Style->Padding.y
    };

    rect MenuItemTextBounds = {
        .Pos   = Pos,
        .Dim.w = MeasureText(Renderer, Text, Style->MainFont, TextPos).w + Style->Padding.w*2,
        .Dim.h = Style->Padding.y*2 + Renderer->Fonts[Style->MainFont].Height
    };

    colorb ButtonColor = Style->DefaultColor;

    if (IsInsideRect(Input->mPos, MenuItemTextBounds)) {
        Ctx->Hot    = Me;
        ButtonColor = Style->HotColor;
        if (Input->mLeft) {
            Ctx->Clicked = Me;
            WasClicked   = 1;
            ButtonColor  = Style->ClickedColor;
        }
    }

    DrawRect(Renderer, MenuItemTextBounds, ButtonColor);
    DrawText(Renderer, Text, Style->MainFont, TextPos, GREY_50);

    Ctx->Last = Me;
    if (Ctx->Current < Ctx->NoItems)
        Ctx->Current++;

    return WasClicked;
}

#if 0
internal f32 uiSlder(ui_ctx *Ctx, ui_style *Style, ui_input *Input, renderer *Renderer, r32 LastValue, rv2 Pos, r32 Range) {
    r32 Value = LastValue*Range;
    id Me = Ctx->Current;

    rect SliderGroove = rect_(Pos.x, Pos.y, Range + Style->SliderHandleWidth, Style->SliderHandleWidth);
    rect SliderHandle = rect_(Pos.x + Value, Pos.y - Style->SliderHandleHeight/2 + Style->SliderHandleWidth/2,
                              Style->SliderHandleWidth, Style->SliderHandleHeight);

    colorb HandleColor = Style->DefaultColor;

    if (IsInsideRect(Input->mPos, rect_Union(SliderGroove, SliderHandle))) {
        Ctx->Hot    = Me;
        HandleColor = Style->HotButtonColor;

        if (Input->dmWheel) {
            Value = Min(Value + (Input->dmWheel/120) * 10, Range);
        }

        if (Input->mLeftButtonIsDown) {
            Ctx->Clicked = Me;
            HandleColor  = Style->ClickedButtonColor;
            Value = Min(Input->mPos.x - Pos.x, Range);
        }
    }

    if (Value < 0)
        Value = 0;

    DrawRect(Renderer, SliderGroove, (colorb){GREY_800});
    DrawRect(Renderer, SliderHandle, HandleColor);

    Ctx->Last = Me;
    if (Ctx->Current < Ctx->NoItems)
        Ctx->Current++;

    return Value/Range;
}
#endif
#endif//UI_H



#ifndef GAP_BUFFER_H
///////////////////////////////////////////////////////////
// GAP BUFFER
#define GAP_BUFFER_H
typedef u32 cursor;
typedef struct _buffer {
    c8 *Data;
    id  Id;

    cursor GapStart;
    cursor GapEnd;
    cursor Point;
    u32    End;
} buffer;

inline u32 GetBufferGapSize(buffer *Buffer) {
    return Buffer->GapEnd - Buffer->GapStart;
}

inline u32 GetBufferLen(buffer *Buffer) {
    return Buffer->End - GetBufferGapSize(Buffer);
}

inline u32 GetCursorIndex(buffer *Buffer, cursor Cursor) {
    return (Cursor < Buffer->GapStart)? Cursor : Cursor + GetBufferGapSize(Buffer);
}

inline void AssertBufferInvariants(buffer *Buffer) {
    Assert(Buffer->Data);
    Assert(Buffer->GapStart <= Buffer->GapEnd);
    Assert(Buffer->GapEnd <= Buffer->End);
}

inline void AssertCursorInvariants(buffer *Buffer, cursor Cursor) {
    Assert(Cursor <= GetBufferLen(Buffer));
}

inline c8 GetBufferChar(buffer *Buffer, cursor Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    return Buffer->Data[GetCursorIndex(Buffer, Cursor)];
}

inline void SetBufferChar(buffer *Buffer, cursor Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    Buffer->Data[GetCursorIndex(Buffer, Cursor)] = Char;
}

internal buffer *CreateBuffer(u32 InitialGapSize) {
    buffer *Buffer = GlobalPlatformApi.AllocateMemory(sizeof(buffer)); {
        Buffer->Data     = GlobalPlatformApi.AllocateMemory(InitialGapSize);
        Buffer->GapStart = 0;
        Buffer->GapEnd   = InitialGapSize;
        Buffer->End      = InitialGapSize;
        Buffer->Point    = 0;
    }

    return Buffer;
}

internal void DeleteBuffer(buffer *Buffer) {
    Buffer->GapStart = 0;
    Buffer->GapEnd   = Buffer->End;
    Buffer->Point    = 0;
}

internal void FreeBuffer(buffer *Buffer) {
    GlobalPlatformApi.FreeMemory(Buffer->Data);
    GlobalPlatformApi.FreeMemory(Buffer);
}

internal cursor MoveBufferPosFoward(buffer *Buffer, u32 Pos) {
    Assert(Pos != Buffer->End);
    Pos++;
    if (Pos == Buffer->GapStart)
        Pos =  Buffer->GapEnd;
    return Pos;
}

internal void ShiftGapToCursor(buffer *Buffer, cursor Cursor) {
    u32 GapSize = GetBufferGapSize(Buffer);
    if (Cursor < Buffer->GapStart) {
        u32 GapDelta = Buffer->GapStart - Cursor;
        Buffer->GapStart -= GapDelta;
        Buffer->GapEnd   -= GapDelta;
        MoveMemory(Buffer->Data + Buffer->GapEnd, Buffer->Data + Buffer->GapStart, GapDelta);
    }
    else
    if (Cursor > Buffer->GapStart) {
        u32 GapDelta = Cursor - Buffer->GapStart;
        MoveMemory(Buffer->Data + Buffer->GapStart, Buffer->Data + Buffer->GapEnd, GapDelta);
        Buffer->GapStart += GapDelta;
        Buffer->GapEnd   += GapDelta;
    }
    Assert(GetBufferGapSize(Buffer) == GapSize);
    AssertBufferInvariants(Buffer);
}

internal void EnsureGapSize(buffer *Buffer, u32 Min) {
    if (GetBufferGapSize(Buffer) < Min) {
        ShiftGapToCursor(Buffer, GetBufferLen(Buffer));
        u32 NewEnd = Max(2 * Buffer->End, Buffer->End + Min - GetBufferGapSize(Buffer));
        void *Temp     = GlobalPlatformApi.AllocateMemory(NewEnd);
        CopyMemory(Temp, Buffer->Data, Buffer->End);
        GlobalPlatformApi.FreeMemory(Buffer->Data);
        Buffer->Data   = Temp;
        Buffer->GapEnd = NewEnd;
        Buffer->End    = NewEnd; 
    }
    Assert(GetBufferGapSize(Buffer) >= Min);
}

internal b32 ReplaceChar(buffer *Buffer, cursor Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor < GetBufferLen(Buffer)) {
        SetBufferChar(Buffer, Cursor, Char);
        return 1;
    }
    else {
        return 0;
    }
}

internal void InsertChar(buffer *Buffer, cursor Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    EnsureGapSize(Buffer, 1);
    ShiftGapToCursor(Buffer, Cursor);
    Buffer->Data[Buffer->GapStart] = Char;
    Buffer->GapStart++;
    if (Buffer->Point >= Cursor) Buffer->Point++;
}

internal b32 DeleteBackwardChar(buffer *Buffer, cursor Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor > 0) {
        ShiftGapToCursor(Buffer, Cursor);
        Buffer->GapStart--;
        if (Buffer->Point >= Cursor) Buffer->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal b32 DeleteFowardChar(buffer *Buffer, cursor Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor < GetBufferLen(Buffer)) {
        ShiftGapToCursor(Buffer, Cursor);
        Buffer->GapEnd++;
        if (Buffer->Point > Cursor) Buffer->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal void SaveBuffer(buffer *Buffer, c8 *Filename) {
    GlobalPlatformApi.WriteFile(Buffer->Data, Buffer->GapStart, Filename, 0);
    GlobalPlatformApi.WriteFile(Buffer->Data + Buffer->GapEnd, Buffer->End - Buffer->GapEnd, Filename, 1);
}

internal void LoadBuffer(buffer *Buffer, c8 *Filename) {
    if (Filename) {
        file File = GlobalPlatformApi.LoadFile(Filename);
        DeleteBuffer(Buffer);
        EnsureGapSize(Buffer, File.Size);
        //todo: unecessary CopyMemory?
        CopyMemory(Buffer->Data, File.Data, File.Size);
        Buffer->GapStart = File.Size;
        GlobalPlatformApi.FreeFile(File);
    }
}

inline cursor GetNextCharCursor(buffer *Buffer, cursor Cursor) {
    if (Cursor < GetBufferLen(Buffer)) {
        Cursor++;
        if (GetBufferChar(Buffer, Cursor) == '\r')
            Cursor++;
    }
    return Cursor;
}

inline cursor GetPrevCharCursor(buffer *Buffer, cursor Cursor) {
    if (Cursor > 0) {
        Cursor--;
        if (GetBufferChar(Buffer, Cursor) == '\r')
            Cursor--;
    }
    return Cursor;
}

inline cursor GetBeginningOfLineCursor(buffer *Buffer, cursor Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    Cursor = GetPrevCharCursor(Buffer, Cursor);
    while (Cursor > 0) {
        c8  Char = GetBufferChar(Buffer, Cursor);
        if (Char == '\n') {
            return GetNextCharCursor(Buffer, Cursor);
        }
        Cursor = GetPrevCharCursor(Buffer, Cursor);
    }
    return 0;
}

inline cursor GetEndOfLineCursor(buffer *Buffer, cursor CurrentCursor) {
    AssertCursorInvariants(Buffer, CurrentCursor);
    while (CurrentCursor < GetBufferLen(Buffer)) {
        c8  Char = GetBufferChar(Buffer, CurrentCursor);
        if (Char == '\n') {
            return CurrentCursor;
        }
        CurrentCursor = GetNextCharCursor(Buffer, CurrentCursor);
    }
    return GetBufferLen(Buffer);
}

inline cursor GetBeginningOfNextLineCursor(buffer *Buffer, cursor CurrentCursor) {
    return GetNextCharCursor(Buffer, GetEndOfLineCursor(Buffer, CurrentCursor));
}

inline cursor GetEndOfPrevLineCursor(buffer *Buffer, cursor CurrentCursor) {
    return GetPrevCharCursor(Buffer, GetBeginningOfLineCursor(Buffer, CurrentCursor));
}

inline cursor GetBeginningOfPrevLineCursor(buffer *Buffer, cursor CurrentCursor) {
    return GetBeginningOfLineCursor(Buffer, GetPrevCharCursor(Buffer, GetBeginningOfLineCursor(Buffer, CurrentCursor)));
}

inline cursor GetCursorColumn(buffer *Buffer, cursor Cursor) {
    return Cursor - GetBeginningOfLineCursor(Buffer, Cursor);
}

inline cursor GetLineLen(buffer *Buffer, cursor Cursor) {
    return GetEndOfLineCursor(Buffer, Cursor) - GetBeginningOfLineCursor(Buffer, Cursor);
}

inline cursor GetBegginingOfBufferCursor(buffer *Buffer, cursor CurrentCursor) {
    return 0;
}

inline cursor GetEndOfBufferCursor(buffer *Buffer, cursor CurrentCursor) {
    return GetBufferLen(Buffer);
}

inline cursor GetBufferColumn(buffer *Buffer, cursor CurrentCursor) {
    return CurrentCursor - GetBeginningOfLineCursor(Buffer, CurrentCursor);
}

inline cursor GetBufferLine(buffer *Buffer, cursor CurrentCursor) {
    u32 Line = 0;
    do {
        CurrentCursor = GetBeginningOfPrevLineCursor(Buffer, CurrentCursor);
        Line++;
    } while (GetBeginningOfPrevLineCursor(Buffer, CurrentCursor) != 0);
    return Line;
}
#endif//GAP_BUFFER_H



#ifndef LEXER_H
///////////////////////////////////////////////////////////
// LEXER
#define LEXER_H
const global u8 CHAR_TO_DIGIT[128] = {
    ['0'] = 0,
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['a'] = 10,
    ['A'] = 10,
    ['b'] = 11,
    ['B'] = 11,
    ['c'] = 12,
    ['C'] = 12,
    ['d'] = 13,
    ['D'] = 13,
    ['e'] = 14,
    ['E'] = 14,
    ['f'] = 15,
    ['F'] = 15,
};

typedef enum _token_type {
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_DELIMITER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_STR,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_NAME,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_WHITESPACE
} token_type;

typedef enum _token_mode {
    TOKEN_MODE_NONE,
    TOKEN_MODE_HEX,
    TOKEN_MODE_OCT,
    TOKEN_MODE_BIN,
    TOKEN_MODE_CHAR
} token_mode;

typedef struct _token {
    token_type Type;
    token_mode Mode;

    u32 Error;

    cursor Start;
    cursor End;

    union {
        u64 iVal;
        f64 fVal;
        c8 *sVal;
        c8 *Name;
    };
    u32 NameLen;
} token;

internal cursor ScanInt(buffer *Buffer, cursor Cursor, token *Token) {
    Token->Start = Cursor;
    u64 Base = 10;
    if (GetBufferChar(Buffer, Cursor) == '0') {
        Cursor++;
        if (tolower(GetBufferChar(Buffer, Cursor)) == 'x') {
            Cursor++;
            Token->Mode = TOKEN_MODE_HEX;
            Base = 16;
        }
        else
        if (tolower(GetBufferChar(Buffer, Cursor)) == 'b') {
            Cursor++;
            Token->Mode = TOKEN_MODE_BIN;
            Base = 2;
        }
        else
        if (isdigit(GetBufferChar(Buffer, Cursor))) {
            Token->Mode = TOKEN_MODE_OCT;
            Base = 8;
        }
    }
    u64 Val = 0;
    while (1) {
        u64 Digit = CHAR_TO_DIGIT[GetBufferChar(Buffer, Cursor)];
        if (Digit == 0 && GetBufferChar(Buffer, Cursor) != '0') {
            break;
        }
        if (Digit > Base) {
            //todo: switch color to some error color or use those squiggly undelines idk
            Token->Error = 1;
        }
        if (Val > (UINT64_MAX - Digit)/Base) {
            //note: see above todo
            Token->Error = 1;
            while (isdigit(GetBufferChar(Buffer, Cursor)))
                Cursor++;
            Val = 0;
        }
        Val = Val * Base + Digit;
        Cursor++;
    }
    Token->End  = Cursor;
    Token->iVal = Val;
    Token->Type = TOKEN_TYPE_INT;

    return Cursor;
}

internal cursor ScanFloat(buffer *Buffer, cursor Cursor, token *Token) {
    cursor Start = Cursor;
    Token->Start = Start;
    while (isdigit(GetBufferChar(Buffer, Cursor))) {
        Cursor++;
    }
    if (GetBufferChar(Buffer, Cursor) == '.') {
        Cursor++;
    }
    Cursor++;
    while (isdigit(GetBufferChar(Buffer, Cursor))) {
        Cursor++;
    }
    if (tolower(GetBufferChar(Buffer, Cursor)) == 'e') {
        Cursor++;
        if (GetBufferChar(Buffer, Cursor) == '+' || GetBufferChar(Buffer, Cursor) == '-') {
            Cursor++;
        }
        if (!isdigit(GetBufferChar(Buffer, Cursor))) {
            Token->Error = 1;
        }
        while (isdigit(GetBufferChar(Buffer, Cursor))) {
            Cursor++;
        }
    }
    // c8 *End = Cursor;
    f64 Val = 0;//strtod(Start, NULL); todo
    if (Val == HUGE_VAL || Val == -HUGE_VAL) {
        Token->Error = 1;
    }
    Token->End  = Cursor;
    Token->fVal = Val;
    Token->Type = TOKEN_TYPE_FLOAT;

    return Cursor;
}

const global c8 ESCAPE_TO_CHAR[128] = {
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['b'] = '\b',
    ['a'] = '\a',
    ['0'] =   0,
};

internal cursor ScanStr(buffer *Buffer, cursor Cursor, token *Token) {
    Assert(GetBufferChar(Buffer, Cursor) == '"');
    Token->Start = Cursor;
    Cursor++;
    char *Str = NULL;
    while (GetBufferChar(Buffer, Cursor) && GetBufferChar(Buffer, Cursor) != '"') {
        char Val = GetBufferChar(Buffer, Cursor);
        if (Val == '\n') {
            Token->Error = 1;
        }
        else
        if (Val == '\\') {
            Cursor++;
            Val = ESCAPE_TO_CHAR[GetBufferChar(Buffer, Cursor)];
            if (Val == 0 && GetBufferChar(Buffer, Cursor) != '0') {
                Token->Error = 1;
            }
        }
        Cursor++;
    }
    if (GetBufferChar(Buffer, Cursor)) {
        Assert(GetBufferChar(Buffer, Cursor) == '"');
        Cursor++;
    }
    else {
        Token->Error = 1;
    }
    Token->End  = Cursor;
    Token->sVal = Str;
    Token->Type = TOKEN_TYPE_STR;

    return Cursor;
}

internal cursor ScanChar(buffer *Buffer, cursor Cursor, token *Token) {
    Token->Start = Cursor;
    Cursor++;
    c8 Val = 0;
    if (GetBufferChar(Buffer, Cursor) == '\'') {
        //
        Cursor++;
    }
    else
    if (GetBufferChar(Buffer, Cursor) == '\n') {
        //
    }
    else
    if (GetBufferChar(Buffer, Cursor) == '\\') {
        Cursor++;
        Val = ESCAPE_TO_CHAR[GetBufferChar(Buffer, Cursor)];
        if (Val == 0 && GetBufferChar(Buffer, Cursor) != '0') {
            //
        }
        Cursor++;
    }
    else {
        Val = GetBufferChar(Buffer, Cursor);
        Cursor++;
    }
    
    if (GetBufferChar(Buffer, Cursor) != '\'') {
        //
    }
    else {
        Cursor++;
    }
    Token->End  = Cursor;
    Token->iVal = Val;
    Token->Type = TOKEN_TYPE_INT;
    Token->Mode = TOKEN_MODE_CHAR;

    return Cursor;
}

#define CASE1(c, c1, k)                                 \
    case c: {                                           \
        Token->Type = GetBufferChar(Buffer, Cursor)++;  \
        if (GetBufferChar(Buffer, Cursor) == c1) {      \
            Token->Type = k;                            \
            Cursor++;                                   \
        }                                               \
        break;                                          \
    }

#define CASE2(c, c1, k, c2, k1)                         \
    case c: {                                           \
        Token->Type = GetBufferChar(Buffer, Cursor)++;  \
        if (GetBufferChar(Buffer, Cursor) == c1) {      \
            Token->Type = k;                            \
            Cursor++;                                   \
        }                                               \
        else                                            \
        if (GetBufferChar(Buffer, Cursor) == c2) {      \
            Token->Type = k1;                           \
            Cursor++;                                   \
        }                                               \
        break;                                          \
    }

internal c8 *GetSubStr(c8 *Start, c8 *End) {
    u32 Len = (i32)(End - Start);
    c8 *Str = GlobalPlatformApi.AllocateMemory(Len + 1);
    CopyMemory(Str, Start, Len);
    Str[Len] = 0;
    return Str;
}

b32 IsKeyword(c8 *Name, u32 NameLen) {
    b32 Result = 0;
    // c8 Temp = Name[NameLen];
    // Name[NameLen] = 0;
    // if (strcmp(Name, "return"))
    //     Result = 1;
    // Name[NameLen] = Temp;
    return Result;
}

internal cursor NextToken(buffer *Buffer, cursor Cursor, token *Token) {
TOP:
    Token->Start = Cursor;
    Token->Mode  = 0;
    switch (GetBufferChar(Buffer, Cursor)) {
        case '\r': case '\v': {
            while (GetBufferChar(Buffer, Cursor) == '\r' || GetBufferChar(Buffer, Cursor) == '\v') {
                Cursor++;
            }
            goto TOP;
            break;
        }
        case ' ': case '\n':
            while (GetBufferChar(Buffer, Cursor) == ' ' || GetBufferChar(Buffer, Cursor) == '\n') {
                Cursor++;
            }
            Token->Type = TOKEN_TYPE_WHITESPACE;
            break;
        case '\'': {
            Cursor = ScanChar(Buffer, Cursor, Token);
            break;
        }
        case '"': {
            Cursor = ScanStr(Buffer, Cursor, Token);
            break;
        }
        case '/': {
            Cursor++;
            if (GetBufferChar(Buffer, Cursor) == '/') {
                while (GetBufferChar(Buffer, Cursor) != '\n')
                    Cursor++;
                Token->Type = TOKEN_TYPE_COMMENT;
            }
            else
            if (GetBufferChar(Buffer, Cursor) == '*') {
                while (GetBufferChar(Buffer, Cursor)     != '*' &&
                       GetBufferChar(Buffer, Cursor + 1) != '/')
                    Cursor++;
                Token->Type = TOKEN_TYPE_COMMENT;
            }
            break;
        }
        case '.': {
            if (isdigit(GetBufferChar(Buffer, Cursor) + 1)) {
                Cursor = ScanFloat(Buffer, Cursor, Token);
            }
            else {
                Cursor++;
                Token->Type = TOKEN_TYPE_OPERATOR;
            }

            break;
        }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            while (isdigit(GetBufferChar(Buffer, Cursor))) {
                Cursor++;
            }
            c8 c = GetBufferChar(Buffer, Cursor);
            Cursor = Token->Start;
            if (c == '.' || tolower(c) == 'e') {
                Cursor = ScanFloat(Buffer, Cursor, Token);
            }
            else {
                Cursor = ScanInt(Buffer, Cursor, Token);
            }
            break;
        }
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y':
        case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y':
        case 'Z':
        case '_': {
            // c8 *Start = Cursor++;
            while (isalnum(GetBufferChar(Buffer, Cursor)) || GetBufferChar(Buffer, Cursor) == '_') {
                Cursor++;
            }
            Token->Name    = "provisory name";
            Token->NameLen = 15;
            Token->Type    = IsKeyword(Token->Name, Token->NameLen) ? TOKEN_TYPE_KEYWORD : TOKEN_TYPE_NAME;
            break;
        }
        case '|':
        case '^':
        case '*':
        case '%':
        case '=':
        case '!':
        case '+':
        case '-':
        case '<':
        case '>':
        case '?':
        case ':':
        case ',': {
            Token->Type = TOKEN_TYPE_OPERATOR;
            Cursor++;
            break;
        }
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case ';': {
            Token->Type = TOKEN_TYPE_DELIMITER;
            Cursor++;
            break;
        }
        default: {
            Token->Type = GetBufferChar(Buffer, Cursor++);
            break;
        }
    }
    Token->End = Cursor;

    return Cursor;
}

inline cursor GetNextTokenCursor(buffer *Buffer, cursor Cursor) {
    cursor Result = Cursor;
    u32    BufferLen = GetBufferLen(Buffer);
    if (GetBufferChar(Buffer, Result) == ' ')
        Result++;
    while (GetBufferChar(Buffer, Result) != ' ' && Result < BufferLen) {
        Result++;
        if (GetBufferChar(Buffer, Result) == '\n')
            break;
    }

    return Result;
}

inline cursor GetPrevTokenCursor(buffer *Buffer, cursor Cursor) {
    cursor Result = Cursor;
    u32    BufferLen = GetBufferLen(Buffer);
    if (GetBufferChar(Buffer, Result) == ' ')
        Result--;
    while (GetBufferChar(Buffer, Result) != ' ' && Result > 0) {
        Result--;
        if (GetBufferChar(Buffer, Result) == '\n')
            break;
    }

    return Result;
}
#endif//LEXER_H



#ifndef COMMANDS_H
///////////////////////////////////////////////////////////
// COMMANDS
#define COMMANDS_H
typedef struct _editor_context {
    renderer *Renderer;

    rv2 mPos;
    r32 dtFrame;
    u16 LastKeyComb;
    c8  LastChar;

    ui_style *uiStyle;
    ui_ctx   *uiCtx;
    ui_input *uiInput;

    c8 *Filename;
    u32 Tab;

    id  CurrentBuffer;
    buffer **Buffers;
    u32 nCurrentBufferLine;
    u32 nCurrentBufferColumn;

    rv2 CurrentCaretPos;
} editor_context;

#define CMD_PROC(Name) void cmd_proc_##Name(editor_context *c)
typedef CMD_PROC(callback);

CMD_PROC(DoNothing) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    //note: does nothing for now, consider error message.
}

CMD_PROC(InsertChar) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point,
               c->LastChar);
}

CMD_PROC(DeleteCharFoward) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    DeleteFowardChar(Buffer, Buffer->Point);
}

CMD_PROC(DeleteCharBackward) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    DeleteBackwardChar(Buffer, Buffer->Point);
}

CMD_PROC(Indent) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point, '\t');
}

CMD_PROC(MoveCarretLeft) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetPrevCharCursor(Buffer, Buffer->Point);
    c->nCurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretRight) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetNextCharCursor(Buffer, Buffer->Point);
    c->nCurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToPrevToken) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetPrevTokenCursor(Buffer, Buffer->Point);
    c->nCurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToNextToken) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetNextTokenCursor(Buffer, Buffer->Point);
    c->nCurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretUp) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    u32 BeginningOfPrevLine = GetBeginningOfPrevLineCursor(Buffer, Buffer->Point);
    u32 PrevLineLen         = GetLineLen(Buffer, BeginningOfPrevLine);
    Buffer->Point = BeginningOfPrevLine + Min(PrevLineLen, c->nCurrentBufferColumn);
    c->nCurrentBufferLine = GetBufferLine(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretDown) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    u32 BeginningOfNextLine = GetBeginningOfNextLineCursor(Buffer, Buffer->Point);
    u32 NextLineLen         = GetLineLen(Buffer, BeginningOfNextLine);
    Buffer->Point = Min(BeginningOfNextLine + Min(NextLineLen, c->nCurrentBufferColumn), GetBufferLen(Buffer) - 1);
    c->nCurrentBufferLine = GetBufferLine(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToLineStart) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point =
        GetBeginningOfLineCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToLineEnd) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetEndOfLineCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToBufferStart) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetBegginingOfBufferCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToBufferEnd) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetEndOfBufferCursor(Buffer, Buffer->Point);
}

CMD_PROC(InsertNewLine) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point, '\n');
}

CMD_PROC(SaveFile) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    SaveBuffer(Buffer, c->Filename);
}

CMD_PROC(OpenFile) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    LoadBuffer(Buffer, c->Filename);
}

typedef struct _command {
    cmd_proc_callback *Proc;
    c8                *Desc;
} command;
#define command_(Proc, Desc) (command){(Proc), (Desc)}

typedef enum _key {
    KEY_NONE = 0, //note:
    KEY_CHAR = 1, //note:
    KEY_DEL,
    KEY_BACK,
    KEY_TAB,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_PG_UP,
    KEY_PG_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_RETURN,
    KEY_CTRL  = 1 << 8,
    KEY_ALT   = 1 << 9,
    KEY_SHIFT = 1 << 10
} key;

#define KeyComb(BaseKey, Ctrl, Alt, Shift) (u16)(BaseKey) | ((u16)(Ctrl) << 8) | ((u16)(Alt)  << 9) | ((u16)(Shift) << 10)
#define Ctrl(Key)  KeyComb(Key, 1, 0, 0)
#define Alt(Key)   KeyComb(Key, 0, 1, 0)
#define Shift(Key) KeyComb(Key, 0, 0, 1)

internal void UpdateEditorContextInput(editor_context *c, platform *p) {
    c->uiInput->mPos = p->mPos;
    c->uiInput->mLeft = p->mLeft;
    c->dtFrame = p->dtForFrame;
    
    u16 Key = KEY_NONE;
    if (p->kDelete)
        Key = KeyComb(KEY_DEL, p->kCtrl, p->kAlt, p->kShift);
    if (p->kBack)
        Key = KeyComb(KEY_BACK, p->kCtrl, p->kAlt, p->kShift);
    if (p->kTab)
        Key = KeyComb(KEY_TAB, p->kCtrl, p->kAlt, p->kShift);
    if (p->kLeft)
        Key = KeyComb(KEY_LEFT, p->kCtrl, p->kAlt, p->kShift);
    if (p->kRight)
        Key = KeyComb(KEY_RIGHT, p->kCtrl, p->kAlt, p->kShift);
    if (p->kUp)
        Key = KeyComb(KEY_UP, p->kCtrl, p->kAlt, p->kShift);
    if (p->kDown)
        Key = KeyComb(KEY_DOWN, p->kCtrl, p->kAlt, p->kShift);
    if (p->kHome)
        Key = KeyComb(KEY_HOME, p->kCtrl, p->kAlt, p->kShift);
    if (p->kPgUp)
        Key = KeyComb(KEY_PG_UP, p->kCtrl, p->kAlt, p->kShift);
    if (p->kPgDown)
        Key = KeyComb(KEY_PG_DOWN, p->kCtrl, p->kAlt, p->kShift);
    if (p->kEnd)
        Key = KeyComb(KEY_END, p->kCtrl, p->kAlt, p->kShift);
    if (p->kReturn)
        Key = KeyComb(KEY_RETURN, p->kCtrl, p->kAlt, p->kShift);
    if (p->kChar && IsPrintableChar(p->Char)) {
        if (!p->kCtrl)
            Key = KEY_CHAR;
        else
            Key = KeyComb(p->Char, p->kCtrl, p->kAlt, p->kShift);
    }
    c->LastChar    = p->Char;
    c->LastKeyComb = Key;
}

#endif//COMMANDS_H

#endif//APP_H