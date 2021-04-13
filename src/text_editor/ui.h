#ifndef UI_H
#define UI_H

#include "lingo.h"
#include "api.h"
#include "maths.h"
#include "graphics.h"

typedef struct _ui_id {
    u32 Owner;
    u32 Item;
} ui_id;

typedef struct _ui_ctx {
    ui_id Hot;
    ui_id Act;

    rv2 mPos;
    b32 mLeft;
    b32 mRight;
} ui_ctx;



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