#ifndef COMMAND_H
#define COMMAND_H

/*
EDITOR_COMMAND_FUNC(CmdFunc_YourFunc) {
    There is a "command_context" implied here.
    You can add whatever you want to that context.
}
keymap *Keymap = CreateKeymap();
Keymap->Commands[GetKeyComb(your, modifiers, and, base key)] = {"your command name", CmdFunc_YourFunc};
*/

#include "lingo.h"
#include "buffer.h"

typedef struct _command_context {
    /* DO NOT REMOVE */ buffer *CurrentBuffer;
    //todo: line number, column number, etc
    c8 LastChar;
} command_context;

#define EDITOR_COMMAND_FUNC(Name) void Name(command_context Ctx)
typedef EDITOR_COMMAND_FUNC(command_func);

typedef struct _command {
    const c8     *Name;
    command_func *Func;
} command;

#define MAX_KEY_COMBS (1 << (8 + 3))
typedef struct _keymap {
    command Commands[MAX_KEY_COMBS];
} keymap;

typedef enum _key {
    KEY_NONE = 0,
    KEY_CHAR,
    KEY_DEL,
    KEY_BACK,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_RETURN
} key;

internal finginline u16 GetKeyComb(b8 Ctrl, b8 Alt, b8 Shift, key Key) {
    return (u16)Key | ((u16)Ctrl << 8) | ((u16)Alt << 9) | ((u16)Shift << 10);
}

internal command *GetKeyCommand(keymap *Keymap, u16 KeyComb) {
    Assert(KeyComb < MAX_KEY_COMBS);
    return Keymap->Commands + KeyComb;
}

EDITOR_COMMAND_FUNC(CmdFunc_DoNothing) {
    //note: does nothing for now, consider error message.
}

internal keymap *CreateKeymap() {
    keymap *Keymap           = AllocateMemory(sizeof(keymap));
    command CommandDoNothing = {"do nothing", CmdFunc_DoNothing};
    for (u32 i = 0; i < MAX_KEY_COMBS; i++) {
        Keymap->Commands[i] = CommandDoNothing;
    }
    return Keymap;
}

internal finginline command NewCommand(c8 *Name, command_func *Func) {
    return (command){Name, Func};
}

EDITOR_COMMAND_FUNC(CmdFunc_InsertChar) {
    InsertChar(Ctx.CurrentBuffer, Ctx.CurrentBuffer->Point, Ctx.LastChar);
}

EDITOR_COMMAND_FUNC(CmdFunc_DeleteCharFoward) {
    DeleteFowardChar(Ctx.CurrentBuffer, Ctx.CurrentBuffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_DeleteCharBackward) {
    DeleteBackwardChar(Ctx.CurrentBuffer, Ctx.CurrentBuffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_MoveCarretLeft) {
    Ctx.CurrentBuffer->Point = GetPrevCursor(Ctx.CurrentBuffer, Ctx.CurrentBuffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_MoveCarretRight) {
    Ctx.CurrentBuffer->Point = GetNextCursor(Ctx.CurrentBuffer, Ctx.CurrentBuffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_MoveCarretToBegginningOfLine) {
    Ctx.CurrentBuffer->Point = GetBegginingOfLineCursor(Ctx.CurrentBuffer, Ctx.CurrentBuffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_MoveCarretToEndOfLine) {
    Ctx.CurrentBuffer->Point = GetEndOfLineCursor(Ctx.CurrentBuffer, Ctx.CurrentBuffer->Point);
}

EDITOR_COMMAND_FUNC(CmdFunc_InsertNewLine) {
    InsertChar(Ctx.CurrentBuffer, Ctx.CurrentBuffer->Point, '\n');
}


internal keymap *CreateDeafaultKeymap() {
    keymap *Keymap = CreateKeymap();
    Keymap->Commands[GetKeyComb(0, 0, 0, KEY_CHAR)]   = NewCommand("insert char",                       CmdFunc_InsertChar);
    Keymap->Commands[GetKeyComb(0, 0, 0, KEY_DEL)]    = NewCommand("delete char foward",                CmdFunc_DeleteCharFoward);
    Keymap->Commands[GetKeyComb(0, 0, 0, KEY_BACK)]   = NewCommand("delete char backward",              CmdFunc_DeleteCharBackward);
    Keymap->Commands[GetKeyComb(0, 0, 0, KEY_LEFT)]   = NewCommand("move carret left",                  CmdFunc_MoveCarretLeft);
    Keymap->Commands[GetKeyComb(0, 0, 0, KEY_RIGHT)]  = NewCommand("move carret right",                 CmdFunc_MoveCarretRight);
    Keymap->Commands[GetKeyComb(0, 0, 0, KEY_HOME)]   = NewCommand("move carret to begginning of line", CmdFunc_MoveCarretToBegginningOfLine);
    Keymap->Commands[GetKeyComb(0, 0, 0, KEY_END)]    = NewCommand("move carret to end of line",        CmdFunc_MoveCarretToEndOfLine);
    // Keymap->Commands[GetKeyComb(0, 0, 0, KEY_RETURN)] = NewCommand("insert new line",                   CmdFunc_InsertNewLine);
    
    return Keymap;
}

#endif//COMMAND_H