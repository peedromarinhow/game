#ifndef UI_H
#define UI_H

#include "lingo.h"
#include "renderer.h"

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

    colorb DefaultTextColor;

    colorb HotButtonColor;
    colorb ClickedButtonColor;
    colorb DefaultButtonColor;

    r32 SliderHandleWidth;
    r32 SliderHandleHeight;
} ui_style;

internal b32 UiAddButton(renderer *Renderer, ui_ctx *Ctx, ui_style *Style, rv2 Pos, c8 *Text) {
    b32 WasClicked = 0;
    id  Me = Ctx->Current;

    rv2 TextPos  = Pos;

    Pos.x += Style->Padding.x;
    Pos.y += Style->Padding.y;

    colorb ButtonColor = Style->DefaultButtonColor;

    rect MenuItemTextBounds = MeasureText(Renderer, Text, Style->Font, TextPos);

    TextPos.x += Style->Padding.x/2;
    TextPos.y += Style->Padding.y/2;

    MenuItemTextBounds.w += Style->Padding.x;
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
            Value = Min(Value + Ctx->dmWheel/10, Range);
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

#endif//UI_H