#ifndef COMMAND_H
#define COMMAND_H

/*
EDITOR_COMMAND_FUNC(CommandFunc_YourFunc) {
    There is a "command_context" implied here.
    You can add whatever you want to that context.
}
keymap *Keymap = CreateKeymap();
Keymap->Commands[GetKeyComb(your, modifiers, and, base key)] = {"your command name", CommandFunc_YourFunc};
*/

#include "lingo.h"
#include "buffer.h"

typedef struct _command_context {
    /* DO NOT REMOVE */ buffer *CurrentBuffer;
    /* DO NOT REMOVE */ input   LastInput;
    //todo: line number, column number, etc
} command_context;

#define EDITOR_COMMAND_FUNC(Name) void Name(command_context Ctx)
typedef EDITOR_COMMAND_FUNC(command_func);

typedef struct _command {
    const c8     *Name;
    command_func *Func;
} command;

#define MAX_KEY_COMBS (1 << (8 + 3))
typedef struct _keymap {
    command Commands[MAX_KEY_COMBS];    //note: this is huge, always pass as pointer
} keymap;

EDITOR_COMMAND_FUNC(CommandFunc_DoNothing) {
    //note: does nothing for now, consider error message.
}

EDITOR_COMMAND_FUNC(CommandFunc_SelfInsertChar) {
    InsertChar(Ctx.CurrentBuffer, Ctx.CurrentBuffer->Point, Ctx.LastInput.Char);
}

internal keymap *CreateKeymap() {
    keymap *Keymap           = AllocateMemory(sizeof(keymap));
    command CommandDoNothing = {"do nothing", CommandFunc_DoNothing};
    for (u32 i = 0; i < MAX_KEY_COMBS; i++) {
        Keymap->Commands[i] = CommandDoNothing;
    }
    return Keymap;
}

internal keymap *CreateDeafaultKeymap() {
    keymap *Keymap                = CreateKeymap();
    command CommandSelfInsertChar = {"self insert character", CommandFunc_SelfInsertChar};
    for (c8 Char = 0; Char < 127; Char++) {
        if (IsPrintableChar(Char)) {
            Keymap->Commands[Char] = CommandSelfInsertChar;
        }
    }
    return Keymap;
}

internal finginline u16 GetKeyComb(b8 Ctrl, b8 Alt, b8 Shift, u8 Key) {
    return ((u16)Ctrl << 0) | ((u16)Alt << 1) | ((u16)Shift << 2) | (u16)(Key << 8);
}

internal command *GetKeyCommand(keymap *Keymap, u16 KeyComb) {
    Assert(KeyComb < MAX_KEY_COMBS);
    return Keymap->Commands + KeyComb;
}

#endif//COMMAND_H