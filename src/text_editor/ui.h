#ifndef UI_H
#define UI_H

#include "lingo.h"
#include "api.h"
#include "maths.h"
#include "graphics.h"

internal b32 IsInsideRect(rv2 Pos, rectf32 Rect) {
    return (Pos.x > Rect.x && Pos.x < Rect.x + Rect.w) && (Pos.y < Rect.y && Pos.y > Rect.y - Rect.h);
}

internal void DrawTextBackGround(font *Font, c8 *Text, rv2 Pos, r32 Padding, color TextColor, color BackColor) {
    rv2 TextDim = GetTextSize(Font, Text, 0, 0, Font->Size, 0, 0);
    rv2 RectDim = rv2_(TextDim.x + Padding, TextDim.y + Padding);
    DrawRect(ORIGIN_TOPLEFT, Pos, RectDim, BackColor);
    DrawText(Font, Text, rv2_(Pos.x + Padding/2, Pos.y - Padding/2 - Font->Size *.75f), Font->Size, 0, 0, TextColor);
}

internal void DrawButton(font *Font, c8 *Text, rv2 Pos, r32 Padding, rv2 MousePos) {
    rv2 TextDim = GetTextSize(Font, Text, 0, 0, Font->Size, 0, 0);
    color BackgroundColor = HexToColor(0x404040FF);
    rectf32 BackgroundRect = (rectf32){Pos.x, Pos.y, TextDim.x + Padding, TextDim.y + Padding};
    if (IsInsideRect(MousePos, BackgroundRect))
        BackgroundColor = HexToColor(0x606060FF);
    DrawRect(ORIGIN_TOPLEFT, Pos, rv2_(BackgroundRect.w, BackgroundRect.h), BackgroundColor);
    DrawText(Font, Text, rv2_(Pos.x + Padding/2, Pos.y - Padding/2 - Font->Size *.75f), Font->Size, 0, 0, HexToColor(0xFAFAFAFF));
}

#endif//UI_H