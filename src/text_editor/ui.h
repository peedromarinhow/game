#ifndef UI_H
#define UI_H

#include "lingo.h"
#include "maths.h"
#include "platform.h"
#include "renderer.h"

//note: inspired by github.com/rxi/microui

//todo:
//  id hashing
//  general input (including keyboard)

enum layout_type {
    ui_LAYOUT_ABSOLUTE,
    ui_LAYOUT_RELATIVE
};

enum ui_color {
    ui_COLOR_BACK,
    ui_COLOR_BASE,
    ui_COLOR_TEXT,
    ui_COLOR_TEXT_INCATIVE,
    ui_COLOR_TEXT_FOCUS,
    ui_COLOR_BUTTON,
    ui_COLOR_BUTTON_HOVER, 
    ui_COLOR_BUTTON_FOCUS,
    ui_NO_COLORS = ui_COLOR_BUTTON_FOCUS + 1
};

typedef struct _ui_layout {
    rect Body;
    i32 ItemWidth;
    i32 ItemHeight;
    rv2 Pos;
    rv2 Dim;
    rv2 Max;
} ui_layout;

typedef struct _ui_style {
    id Font;
    id MonoFont;
    i32 Padding;
    colorb Colors[ui_NO_COLORS];
} ui_style;

typedef struct _ui_context {
    ui_style Style;
    id Hover;
    id Focus;
    id LastId;
    id Current;
    i16 NoItems;
    b32 MouseDown;
    u32 MouseWich;
    b32 KeyDown;
    u32 KeyWich;
    rv2 MousePos;
    ui_layout Layout;
    c8 TextInput[32];
} ui_context;

enum ui_opt {
    ui_OPT_TEXT_ALIGN_CENTER = 0b1,
    ui_OPT_TEXT_ALIGN_RIGHT  = 0b10,
    ui_OPT_NO_INTERACTION    = 0b100,
    ui_OPT_HOLDFOCUS         = 0b1000
};

enum ui_result {
    ui_RESULT_CHANGE = 0b1,
    ui_RESULT_SUBMIT = 0b10
};

enum ui_min_dims {
    ui_MIN_BUTTON_WIDTH  = 64,
    ui_MIN_BUTTON_HEIGHT = 36,
    ui_MIN_TEXTBOX_WIDTH  = 280,
    ui_MIN_TEXTBOX_HEIGHT = 48,
    ui_MIN_SNACKBAR_WIDTH  = 344,
    ui_MIN_SNACKBAR_HEIGHT = 48
};

enum ui_mouse_buttons {
    ui_MOUSE_BUTTON_LEFT  = 0b1,
    ui_MOUSE_BUTTON_RIGHT = 0b10
};

enum ui_key {
    ui_KEY_BACKSPACE = 0b1,
    ui_KEY_RETURN    = 0b10
};

internal id ui_GetId(ui_context *Ctx) {
    id Result = Ctx->Current;
    Ctx->LastId = Result;
    if (Ctx->Current < Ctx->NoItems)
        Ctx->Current++;
    return Result;
}

internal colorb ui_GetColor(ui_context *Ctx, id ColorId) {
    return Ctx->Style.Colors[ColorId];
}

internal void ui_NextRow(ui_context *Ctx) {
    ui_layout *Layout = &Ctx->Layout;
    Layout->Pos.y -= Layout->ItemHeight + Ctx->Style.Padding;
    Layout->Pos.x  = Layout->Body.x;
}

internal rect ui_NextRect(ui_context *Ctx, i32 Width, i32 Height) {
    ui_layout *Layout = &Ctx->Layout;
    ui_style  *Style  = &Ctx->Style;
    rect Result;

    Result.w = Layout->ItemWidth  = Width;
    Result.h = Layout->ItemHeight = Height;

    Result.x = Layout->Pos.x;
    Result.y = Layout->Pos.y + Layout->Dim.h - Layout->ItemHeight;

    Layout->Pos.x += Result.w + Style->Padding;

    Layout->Max.x = Max(Layout->Max.x, Result.x + Result.w);
    Layout->Max.y = Max(Layout->Max.y, Result.y + Result.h);

    return Result;
}

internal void ui_UpdateControls(ui_context *Ctx, id Id, rect Rect, u32 Opts) {
    //todo: figure out how to snap focus and how to hold focus
    if (Opts & ui_OPT_NO_INTERACTION)
        return;

    b32 MouseIsOver = IsInsideRect(Ctx->MousePos, Rect);
    if (MouseIsOver)
        Ctx->Hover = Id;
    else
        Ctx->Hover = 0;

    if (Ctx->Focus == Id) {
        if (!MouseIsOver)
            Ctx->Focus = 0;
    }

    if (Ctx->Hover == Id) {
        if (Ctx->MouseDown)
            Ctx->Focus = Id;
        else
            Ctx->Focus = 0;
    }
}

internal void ui_DrawRect(renderer *Renderer, ui_context *Ctx, id Id, rect Rect, id ColorId, u32 Opts) {
    if (Id == Ctx->Focus)
        ColorId = ui_COLOR_BUTTON_FOCUS;
    else
    if (Id == Ctx->Hover)
        ColorId = ui_COLOR_BUTTON_HOVER;
    DrawRect(Renderer, Rect, ui_GetColor(Ctx, ColorId));
}

internal void ui_DrawText(renderer *Renderer, ui_context *Ctx, c8 *Text, rect Rect, id ColorId, u32 Opts) {
    rv2 Dim = MeasureText(Renderer, Text, Ctx->Style.Font);
    rv2 Pos;
    Pos.y = Rect.y + (Rect.h - Dim.h) / 2;
    if (Opts & ui_OPT_TEXT_ALIGN_CENTER)
        Pos.x = Rect.x + (Rect.w - Dim.w) / 2;
    else
    if (Opts & ui_OPT_TEXT_ALIGN_RIGHT)
        Pos.x = (Rect.x + Rect.w) - Dim.w - Ctx->Style.Padding;
    else
        Pos.x = Rect.x + Ctx->Style.Padding;

    // if (Dim.w + Ctx->Style.Padding * 2 < Rect.w)
    //todo: handle text overflow i.e: long_text -> long_te..
    DrawText(Renderer, Text, Ctx->Style.Font, Pos, ui_GetColor(Ctx, ColorId));
}

internal void ui_Label(renderer* Renderer, ui_context *Ctx, c8 *Text, u32 Opts) {
    rect Rect = ui_NextRect(Ctx, ui_MIN_BUTTON_WIDTH, ui_MIN_BUTTON_HEIGHT);
    ui_DrawText(Renderer, Ctx, Text, Rect, ui_COLOR_TEXT, Opts);
}

internal b32 ui_Button(renderer* Renderer, ui_context *Ctx, c8 *Text, u32 Opts) {
    b32 Result = 0;

    r32 Width = MeasureText(Renderer, Text, 0).w;
    Width = Width + 2*Ctx->Style.Padding < ui_MIN_BUTTON_WIDTH? ui_MIN_BUTTON_WIDTH : Width + 2*Ctx->Style.Padding;
    
    id Id = ui_GetId(Ctx);
    rect Rect = ui_NextRect(Ctx, Width, ui_MIN_BUTTON_HEIGHT);

    ui_UpdateControls(Ctx, Id, Rect, 0);

    if (Ctx->MouseDown && Ctx->Focus == Id)
        Result = 1;

    ui_DrawRect(Renderer, Ctx, Id, Rect, ui_COLOR_BUTTON, Opts);
    if (Text)
        ui_DrawText(Renderer, Ctx, Text, Rect, ui_COLOR_TEXT, Opts | ui_OPT_TEXT_ALIGN_CENTER);
    return Result;
}

internal void ui_TextInput(ui_context *Ctx, c8 *Text) {
    u32 Len  = ArrayCount(Ctx->TextInput);
    u32 Size = ArrayCount(Text) + 1;
    Assert(Len + Size <= sizeof(Ctx->TextInput));
    // Cop]yMemory(Ctx->TextInput + Len, Text, Size);
}

internal b32 ui_TextBox(renderer *Renderer, ui_context *Ctx, c8 *Buff, u32 BuffLen, u32 Opts) {
    b32 Result = 0;

    id Id = ui_GetId(Ctx);
    rect Rect = ui_NextRect(Ctx, ui_MIN_TEXTBOX_WIDTH, ui_MIN_TEXTBOX_HEIGHT);

    ui_UpdateControls(Ctx, Id, Rect, Opts | ui_OPT_HOLDFOCUS);

    if (Ctx->Focus == Id) {
        u32 Len = ArrayCount(Buff);
        u32 n = Min(BuffLen - Len - 1, ArrayCount(Ctx->TextInput));
        if (n > 0) {
            // CopyMemory(Buff + Len, Ctx->TextInput, n);
            Len += n;
            Buff[Len] = '\0';
            Result |= ui_RESULT_CHANGE;
        }
        if (Ctx->KeyWich & ui_KEY_BACKSPACE && Len > 0) {
            while ((Buff[--Len] & 0xC0) == 0x80 && Len > 0);
            Buff[Len] = '\0';
            Result |= ui_RESULT_CHANGE;
        }
        if (Ctx->KeyWich & ui_KEY_RETURN) {
            // Ctx->Focus = 0;
            Result |= ui_RESULT_SUBMIT;
        }
    }

    ui_DrawRect(Renderer, Ctx, Id, rect_(Rect.x, Rect.y, Rect.w, 2), ui_COLOR_BUTTON, Opts);
    // if (Ctx->Focus == Id) {
    //     ui_style *Style = &Ctx->Style;
    //     colorb Color = Style->Colors[ui_COLOR_TEXT];
    //     id  Font = Style->Font;
    //     rv2 Dim = MeasureText(Renderer, Buff, Font);
    //     rv2 Pos = rv2_(Rect.x + Min(Rect.w - Style->Padding - Dim.w - 1, Style->Padding),
    //                    Rect.y + (Rect.h - Dim.h) / 2);
    //     DrawText(Renderer, Buff, Font, Pos, Color);
    //     DrawRect(Renderer, rect_(Pos.x + Dim.w, Pos.y, 1, Dim.h), Color);
    // }
    // else {
        ui_DrawText(Renderer, Ctx, "Temp", Rect, ui_COLOR_TEXT, Opts);
    // }

    return Result;
}

internal void ui_Snackbar(renderer* Renderer, ui_context *Ctx, c8 *Text, u32 Opts) {
    b32 Result = 0;

    rect Rect = rect_((Renderer->TargetClipRect.w - ui_MIN_SNACKBAR_WIDTH)/2, Ctx->Style.Padding, ui_MIN_SNACKBAR_WIDTH, ui_MIN_SNACKBAR_HEIGHT);

    DrawRect(Renderer, Rect, Ctx->Style.Colors[ui_COLOR_BUTTON]);
    if (Text)
        ui_DrawText(Renderer, Ctx, Text, Rect, ui_COLOR_TEXT, Opts);
}
#endif