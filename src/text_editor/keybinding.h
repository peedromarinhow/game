#ifndef KEYBINDING_H
#define KEYBINDING_H
#include "lingo.h"
#include "platform.h"
#include "gbuff.h"
typedef struct _keybind_command_context {
    gbuff *CurrentBuff;
    platform_api *Api;
    u32 Col;
    u32 Line;
    c8 Char;
} keybind_command_context;

#define KEYBIND_COMMAND(Name) void keybind_command_##Name(keybind_command_context *Ctx)
typedef KEYBIND_COMMAND(callback);

typedef struct _keybind_command {
    keybind_command_callback *Proc;
} keybind_command;
#define keybind_command_(Proc) (keybind_command){(keybind_command_##Proc)}

enum keybind_keys {
    keybind_KEY_NONE = 0, //note:
    keybind_KEY_CHAR = 1, //note:
    keybind_KEY_DEL,
    keybind_KEY_BACK,
    keybind_KEY_TAB,
    keybind_KEY_LEFT,
    keybind_KEY_RIGHT,
    keybind_KEY_UP,
    keybind_KEY_DOWN,
    keybind_KEY_PG_UP,
    keybind_KEY_PG_DOWN,
    keybind_KEY_HOME,
    keybind_KEY_END,
    keybind_KEY_RETURN,
    keybind_KEY_CTRL  = 1 << 8,
    keybind_KEY_ALT   = 1 << 9,
    keybind_KEY_SHIFT = 1 << 10
};

#define KeyComb(Base, Ctrl, Alt, Shift) \
    (u16)(Base)        |                \
   ((u16)(Ctrl)  << 8) |                \
   ((u16)(Alt)   << 9) |                \
   ((u16)(Shift) << 10)

KEYBIND_COMMAND(DoNothing) {
    return;
}

KEYBIND_COMMAND(BuffInsertChar) {
    if (Ctx->Char != '\b' && Ctx->Char != '\r')
        gbuff_InsertChar(Ctx->Api, Ctx->CurrentBuff, Ctx->CurrentBuff->Point, Ctx->Char);
}

KEYBIND_COMMAND(BuffMoveCursorLeft) {
    gbuff *Buff = Ctx->CurrentBuff;
    Buff->Point = gbuff_GetPrevCharCursor(Buff, Buff->Point);
    Ctx->Col = gbuff_GetColumn(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffMoveCursorRight) {
    gbuff *Buff = Ctx->CurrentBuff;
    Buff->Point = gbuff_GetNextCharCursor(Buff, Buff->Point);
    Ctx->Col = gbuff_GetColumn(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffMoveCursorUp) {
    gbuff *Buff = Ctx->CurrentBuff;
    u32 BeginningOfPrevLine = gbuff_GetBeginningOfPrevLineCursor(Buff, Buff->Point);
    u32 PrevLineLen         = gbuff_GetLineLen(Buff, BeginningOfPrevLine);
    Buff->Point = BeginningOfPrevLine + Min(PrevLineLen,  Ctx->Col);
    Ctx->Line =  Ctx->Line > 0?  Ctx->Line - 1 : 0;
}

KEYBIND_COMMAND(BuffMoveCursorDown) {
    gbuff *Buff = Ctx->CurrentBuff;
    u32 BeginningOfNextLine = gbuff_GetBeginningOfNextLineCursor(Buff, Buff->Point);
    u32 NextLineLen         = gbuff_GetLineLen(Buff, BeginningOfNextLine);
    Buff->Point = BeginningOfNextLine + Min(NextLineLen,  Ctx->Col);
    Ctx->Line++;
}

KEYBIND_COMMAND(BuffMoveCursorToLineStart) {
    gbuff *Buff = Ctx->CurrentBuff;
    Buff->Point = gbuff_GetBeginningOfLineCursor(Buff, Buff->Point);
    Ctx->Col = gbuff_GetColumn(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffMoveCursorToLineEnd) {
    gbuff *Buff = Ctx->CurrentBuff;
    Buff->Point = gbuff_GetEndOfLineCursor(Buff, Buff->Point);
    Ctx->Col = gbuff_GetColumn(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffDeleteFowardChar) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_DeleteBackwardChar(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffDeleteBackwardChar) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_DeleteFowardChar(Buff, Buff->Point);
}

KEYBIND_COMMAND(BuffNewline) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_InsertChar(Ctx->Api, Buff, Buff->Point, '\n');
    Ctx->Line++;
}

KEYBIND_COMMAND(BuffOpen) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_Load(Ctx->Api, Buff, "a.c");
}

KEYBIND_COMMAND(BuffSave) {
    gbuff *Buff = Ctx->CurrentBuff;
    gbuff_Save(Ctx->Api, Buff, "a.c");
}
#endif//KEYBINDING_H