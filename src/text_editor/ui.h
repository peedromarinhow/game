#ifndef UI_H
#define UI_H

#include "lingo.h"
#include "api.h"
#include "maths.h"
#include "graphics.h"

internal b32 IsInsideRect(rv2 Pos, rectf32 Rect) {
        return (Pos.x > Rect.x && Pos.x < Rect.x + Rect.w) &&
               (Pos.y < Rect.y && Pos.y > Rect.y - Rect.h);
}

internal void DrawButton(font *Font, rv2 Pos, c8 *Text, rv2 MousePos) {
    rv2     TextDim = GetTextSize(Font, Text, Font->Size, 0, 0);
    color   Color   = HexToColor(0x404040FF);
    rectf32 Rect    = rectf32_(Pos.x, Pos.y, TextDim.x + 50, TextDim.y + 50);
    if (IsInsideRect(MousePos, Rect))
        Color = HexToColor(0x606060FF);
    DrawRect(ORIGIN_TOPLEFT, Pos, rv2_(Rect.w, Rect.h), Color);
    DrawText(Font, Text, rv2_(Pos.x + 25, Pos.y - 25 - Font->Size *.75f), Font->Size, 0, 0, HexToColor(0xFAFAFAFF));
}

#endif//UI_H