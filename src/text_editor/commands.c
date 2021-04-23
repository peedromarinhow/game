#if 0
#ifndef COMMAND_H
#define COMMAND_H

#include "lingo.h"
#include "text_buffer.h"

typedef struct _command_context {
    /* DO NOT REMOVE */
        buffer *CurrentBuffer;
        c8      LastChar;
        u32     GoalColumn;
        //todo: line number, column number, etc
    /* DO NOT REMOVE */
} command_context;

//note: how should I do this?
#define COMMAND_FUNC(Name) void CmdFunc_##Name(command_context *Ctx)
typedef void command_func(command_context *Ctx);

typedef struct _command {
    c8           *Desc;
    command_func *Func;
} command;

typedef enum _key {
    KEY_NONE = 0,
    KEY_DEL,
    KEY_BACK,
    KEY_TAB,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_PG_UP,
    KEY_PG_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_RETURN,
    KEY_CHAR,
} key;

#define MAX_KEY_COMBS (1 << (8 + 3))
typedef struct _keymap {
    command Commands[MAX_KEY_COMBS];

} keymap;

internal command *GetKeyCommand(keymap *Keymap, u16 KeyComb) {
    Assert(KeyComb < MAX_KEY_COMBS);
    return Keymap->Commands + KeyComb;
}

internal finginline command NewCommand(command_func *Func, c8 *Description) {
    return (command){Description, Func};
}

#define Bind(k, KeyComb, CommandFunc, CommandDesc) \
    k->Commands[KeyComb] = NewCommand(CommandFunc, CommandDesc);

internal finginline u16 GetKeyComb(b8 Ctrl, b8 Alt, b8 Shift, key Key) {
    return (u16)Key | ((u16)Ctrl << 8) | ((u16)Alt << 9) | ((u16)Shift << 10);
}

internal finginline u16 Ctrl(u8 Key) {
    return GetKeyComb(1, 0, 0, Key);
}

internal finginline u16 Alt(u8 Key) {
    return GetKeyComb(0, 1, 0, Key);
}

internal finginline u16 Sh1ft(u8 Key) {
    return GetKeyComb(0, 0, 1, Key);
}

///////////////////////////////////////////////////////////

COMMAND_FUNC(DoNothing) {
    buffer *Buffer = Ctx->CurrentBuffer;
    //note: does nothing for now, consider error message.
}

COMMAND_FUNC(InsertChar) {
    buffer *Buffer = Ctx->CurrentBuffer;
    InsertChar(Buffer, Buffer->Point,
               Ctx->LastChar);
}

COMMAND_FUNC(DeleteCharFoward) {
    buffer *Buffer = Ctx->CurrentBuffer;
    DeleteFowardChar(Buffer, Buffer->Point);
}

COMMAND_FUNC(DeleteCharBackward) {
    buffer *Buffer = Ctx->CurrentBuffer;
    DeleteBackwardChar(Buffer, Buffer->Point);
}

COMMAND_FUNC(Indent) {
    buffer *Buffer = Ctx->CurrentBuffer;
    InsertChar(Buffer, Buffer->Point, '\t');
}

COMMAND_FUNC(MoveCarretLeft) {
    buffer *Buffer = Ctx->CurrentBuffer;
    Buffer->Point = GetPrevCharCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(MoveCarretRight) {
    buffer *Buffer = Ctx->CurrentBuffer;
    Buffer->Point = GetNextCharCursor(Buffer, Buffer->Point);
}

global u32 GoalColumn = -1;

COMMAND_FUNC(MoveCarretUp) {
    buffer *Buffer = Ctx->CurrentBuffer;
    if (GoalColumn == -1)
        GoalColumn = GetBufferColumn(Buffer, Buffer->Point);
    u32 BeginningOfPrevLine = GetBeginningOfPrevLineCursor(Buffer, Buffer->Point);
    u32 PrevLineLen         = GetLineLen(Buffer, BeginningOfPrevLine);
    Buffer->Point = BeginningOfPrevLine + Min(PrevLineLen, GoalColumn);
}

COMMAND_FUNC(MoveCarretDown) {
    buffer *Buffer = Ctx->CurrentBuffer;
    if (GoalColumn == -1)
        GoalColumn = GetBufferColumn(Buffer, Buffer->Point);
    u32 BeginningOfNextLine = GetBeginningOfNextLineCursor(Buffer, Buffer->Point);
    u32 NextLineLen         = GetLineLen(Buffer, BeginningOfNextLine);
    Buffer->Point = Min(BeginningOfNextLine + Min(NextLineLen, GoalColumn), GetBufferLen(Buffer) - 1);
}

COMMAND_FUNC(MoveCarretToBeginningOfLine) {
    buffer *Buffer = Ctx->CurrentBuffer;
    Buffer->Point =
        GetBeginningOfLineCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(MoveCarretToEndOfLine) {
    buffer *Buffer = Ctx->CurrentBuffer;
    Buffer->Point = GetEndOfLineCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(MoveCarretToBeginningOfBuffer) {
    buffer *Buffer = Ctx->CurrentBuffer;
    Buffer->Point = GetBegginingOfBufferCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(MoveCarretToEndOfBuffer) {
    buffer *Buffer = Ctx->CurrentBuffer;
    Buffer->Point = GetEndOfBufferCursor(Buffer, Buffer->Point);
}

COMMAND_FUNC(InsertNewLine) {
    buffer *Buffer = Ctx->CurrentBuffer;
    InsertChar(Buffer, Buffer->Point, '\n');
}

COMMAND_FUNC(NextBuffer) {
    // Ctx->CurrentBuffer->Active = 0;
    // if (Ctx->CurrentBuffer < Ctx->NoBuffers - 1)
    //     Ctx->CurrentBuffer++;
    // Ctx->CurrentBuffer->Active = 1;
}

COMMAND_FUNC(PrevBuffer) {
    // Ctx->CurrentBuffer->Active = 0;
    // if (Ctx->CurrentBuffer > 0)
    //     Ctx->CurrentBuffer--;
    // Ctx->CurrentBuffer->Active = 1;
}

COMMAND_FUNC(SaveBuffer) {
    buffer *Buffer = Ctx->CurrentBuffer;
    // SaveBuffer(Buffer, Ctx->Filename);
}

COMMAND_FUNC(LoadBuffer) {
    buffer *Buffer = Ctx->CurrentBuffer;
    // LoadBuffer(Buffer, Ctx->Filename);
}

internal keymap *CreateKeymap() {
    keymap *Keymap           = GlobalPlatformApi.AllocateMemory(sizeof(keymap));
    command CommandDoNothing = {"do nothing", CmdFunc_DoNothing};
    for (u32 i = 0; i < MAX_KEY_COMBS; i++) {
        Keymap->Commands[i] = CommandDoNothing;
    }
    return Keymap;
}

internal keymap *CreateMyKeymap() {
    keymap *Keymap = CreateKeymap();

    for (c8 Key = 0; Key < 127; Key++) {
        if (IsPrintableChar(Key)) {
            Bind(Keymap, GetKeyComb(0, 0, 0, Key), CmdFunc_InsertChar, "insert char");
            Bind(Keymap, GetKeyComb(0, 0, 1, Key), CmdFunc_InsertChar, "insert char");
        }
    }

    Bind(Keymap, KEY_DEL,           CmdFunc_DeleteCharFoward,              "delete char foward");
    Bind(Keymap, KEY_BACK,          CmdFunc_DeleteCharBackward,            "delete char backward");
    Bind(Keymap, KEY_TAB,           CmdFunc_Indent,                        "indent");
    Bind(Keymap, KEY_LEFT,          CmdFunc_MoveCarretLeft,                "move carret left");
    Bind(Keymap, KEY_RIGHT,         CmdFunc_MoveCarretRight,               "move carret right");
    Bind(Keymap, KEY_UP,            CmdFunc_MoveCarretUp,                  "move carret left");
    Bind(Keymap, KEY_DOWN,          CmdFunc_MoveCarretDown,                "move carret right");
    Bind(Keymap, Ctrl(KEY_PG_UP),   CmdFunc_NextBuffer,                    "use next buffer");
    Bind(Keymap, Ctrl(KEY_PG_DOWN), CmdFunc_PrevBuffer,                    "use prev buffer");
    Bind(Keymap, KEY_HOME,          CmdFunc_MoveCarretToBeginningOfLine,   "move carret to begginning of line");
    Bind(Keymap, KEY_END,           CmdFunc_MoveCarretToEndOfLine,         "move carret to end of line");
    Bind(Keymap, Ctrl(KEY_HOME),    CmdFunc_MoveCarretToBeginningOfBuffer, "move carret to begginning of buffer");
    Bind(Keymap, Ctrl(KEY_END),     CmdFunc_MoveCarretToEndOfBuffer,       "move carret to end of buffer");
    Bind(Keymap, KEY_RETURN,        CmdFunc_InsertNewLine,                 "insert new line");
    Bind(Keymap, Ctrl('S'),         CmdFunc_SaveBuffer,                    "save buffer");
    Bind(Keymap, Ctrl('O'),         CmdFunc_LoadBuffer,                    "load buffer");
    
    return Keymap;
}

#endif//COMMAND_H
#endif
