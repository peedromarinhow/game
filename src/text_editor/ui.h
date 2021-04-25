#ifndef UI_H
#define UI_H

#include <stdio.h>

#include "lingo.h"
#include "renderer.h"

typedef struct _ui_ctx {
    renderer *Renderer;

    rv2 mPos;
    b32 mLeftButtonIsDown;
    i16 dmWheel;
    //todo: get rid of these

    id Hot;
    id Clicked;
    id Last;
    id Current;

    r32 Point;
} ui_ctx;

typedef struct _ui_style {
    id Font;
    id MonoFont;
    id IconFont;

    r32 CharSpacing;
    r32 LineSpacing;

    rv2 Padding;

    colorb BarColor;

    colorb DefaultTextColor;

    colorb HotButtonColor;
    colorb ClickedButtonColor;
    colorb DefaultButtonColor;

    r32 SliderHandleWidth;
    r32 SliderHandleHeight;
} ui_style;

internal b32 uiButton(ui_ctx *Ctx, ui_style *Style, rv2 Pos, c8 *Text) {
    renderer *Renderer = Ctx->Renderer;
    b32 WasClicked = 0;
    id Me = Ctx->Current;

    rect MenuItemTextBounds = {0};// = MeasureText(Renderer, Text, Style->Font, TextPos);
    MenuItemTextBounds.Pos = Pos;

    rv2 TextPos = Pos;

    Pos.x += Style->Padding.x;
    Pos.y += Style->Padding.y;

    colorb ButtonColor = Style->DefaultButtonColor;

    TextPos.x += Style->Padding.x/2;
    TextPos.y += Style->Padding.y/2;

    MenuItemTextBounds.w += MeasureText(Renderer, Text, Style->Font, TextPos).w + Style->Padding.x;
    MenuItemTextBounds.h  = Style->Padding.y                      +
                            Renderer->Fonts[Style->Font].Ascender +
                            Renderer->Fonts[Style->Font].Descender;

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
    DrawText(Renderer, Text, Style->Font, TextPos, Style->DefaultTextColor);

    Ctx->Last = Me;
    Ctx->Current++; //todo: check for overflow.

    return WasClicked;
}

internal f32 uiSlder(ui_ctx *Ctx, ui_style *Style, r32 LastValue, rv2 Pos, r32 Range) {
    renderer *Renderer = Ctx->Renderer;
    r32 Value = LastValue*Range;
    id Me = Ctx->Current;

    rect SliderGroove = rect_(Pos.x, Pos.y, Range + Style->SliderHandleWidth, Style->SliderHandleWidth);
    rect SliderHandle = rect_(Pos.x + Value, Pos.y - Style->SliderHandleHeight/2 + Style->SliderHandleWidth/2,
                              Style->SliderHandleWidth, Style->SliderHandleHeight);

    colorb HandleColor = Style->DefaultButtonColor;

    if (IsInsideRect(Ctx->mPos, rect_Union(SliderGroove, SliderHandle))) {
        Ctx->Hot    = Me;
        HandleColor = Style->HotButtonColor;

        if (Ctx->dmWheel) {
            Value = Min(Value + (Ctx->dmWheel/120) * 10, Range);
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



internal void uiBottomBar(ui_ctx *Ctx, ui_style *Style, c8 *Filename, u32 nLine, u32 nColumn, r32 dtFrame) {
    renderer *Renderer = Ctx->Renderer;
    id Me = Ctx->Current;

    rv2 Pos = rv2_(0, 0);

    colorb BackgroundColor = Style->DefaultButtonColor;

    rect BarBackground;
    BarBackground.w = Ctx->Renderer->TargetClipRect.w;
    BarBackground.h = Style->Padding.y                      +
                      Renderer->Fonts[Style->Font].Ascender +
                      Renderer->Fonts[Style->Font].Descender;
    BarBackground.Pos = Pos;
    DrawRect(Renderer, BarBackground, BackgroundColor);

    r32 x = 0;
    uiButton(Ctx, Style, rv2_(x, 0), Filename);
    x += MeasureText(Renderer, Filename, Style->Font, rv2_(0, 0)).w + Style->Padding.x*1.5f;
    c8 TextBuffer[32];
    sprintf_s(TextBuffer, 32, "%u, %u", nLine, nColumn);
    uiButton(Ctx, Style, rv2_(x, 0), TextBuffer);
    x = Renderer->TargetClipRect.w - MeasureText(Renderer, "0.000000", Style->Font, rv2_(0, 0)).w - Style->Padding.x;
    sprintf_s(TextBuffer, 32, "%f", dtFrame);
    uiButton(Ctx, Style, rv2_(x, 0), TextBuffer);
}

#endif//UI_H