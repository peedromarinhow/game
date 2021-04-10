#ifndef UI_H
#define UI_H

#include "lingo.h"
#include "api.h"
#include "maths.h"
#include "graphics.h"

internal b32 IsInsideRect(rv2 Pos, rectf32 Rect) {
    return (Pos.x > Rect.x && Pos.x < Rect.x + Rect.w) && (Pos.y < Rect.y && Pos.y > Rect.y - Rect.h);
}

internal void DrawMenu(font *Font, rv2 Pos, rv2 MousePos) {
    c8 *Text    = "This is a \"button\"";
    rv2 TextDim = GetTextSize(Font, Text, Font->Size, 0, 0);
    color BackgroundColor = HexToColor(0x404040FF);
    rectf32 BackgroundRect = (rectf32){Pos.x, Pos.y, TextDim.x + 50, TextDim.y + 50};
    if (IsInsideRect(MousePos, BackgroundRect))
        BackgroundColor = HexToColor(0x606060FF);
    DrawRect(ORIGIN_TOPLEFT, Pos, rv2_(BackgroundRect.w, BackgroundRect.h), BackgroundColor);
    DrawText_(Font, Text, rv2_(Pos.x + 25, Pos.y - 25 - Font->Size *.75f), Font->Size, 0, 0, HexToColor(0xFAFAFAFF));
}

#endif//UI_H