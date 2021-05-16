#ifndef APP_H
#define APP_H

#include "renderer.h"

#ifndef UI_H
///////////////////////////////////////////////////////////
// UI
#define UI_H

#include <stdio.h>
#include "lingo.h"

typedef struct _ui_ctx {
    id  Hot;
    id  Clicked;
    id  Last;
    id  Current;
    u32 NoItems;
} ui_ctx;

typedef struct _ui_style {
    id MainFont;
    id MonoFont;
    id IconFont;
    // rv2 TextSpacing;

    colorb BackColor;
    colorb HotColor;
    colorb ClickedColor;
    colorb DefaultColor;
    
    rv2 Padding;
} ui_style;

typedef struct _ui_input {
    rv2 mPos;
    b32 mLeft;
    b32 mRight;
    i16 mWheel;
} ui_input;

internal b32 uiButton(ui_ctx *Ctx, ui_style *Style, ui_input *Input, renderer *Renderer, rv2 Pos, c8 *Text) {
    b32 WasClicked = 0;
    id  Me = Ctx->Current;

    rv2 TextPos = {
        .x = Pos.x + Style->Padding.x,
        .y = Pos.y + Style->Padding.y
    };

    rect MenuItemTextBounds = {
        .Pos   = Pos,
        .Dim.w = MeasureText(Renderer, Text, Style->MainFont, TextPos).w + Style->Padding.w*2,
        .Dim.h = Style->Padding.y*2 + Renderer->Fonts[Style->MainFont].Height
    };

    colorb ButtonColor = Style->DefaultColor;

    if (IsInsideRect(Input->mPos, MenuItemTextBounds)) {
        Ctx->Hot    = Me;
        ButtonColor = Style->HotColor;
        if (Input->mLeft) {
            Ctx->Clicked = Me;
            WasClicked   = 1;
            ButtonColor  = Style->ClickedColor;
        }
    }

    DrawRect(Renderer, MenuItemTextBounds, ButtonColor);
    DrawText(Renderer, Text, Style->MainFont, TextPos, GREY_50);

    Ctx->Last = Me;
    if (Ctx->Current < Ctx->NoItems)
        Ctx->Current++;

    return WasClicked;
}

#if 0
internal f32 uiSlder(ui_ctx *Ctx, ui_style *Style, ui_input *Input, renderer *Renderer, r32 LastValue, rv2 Pos, r32 Range) {
    r32 Value = LastValue*Range;
    id Me = Ctx->Current;

    rect SliderGroove = rect_(Pos.x, Pos.y, Range + Style->SliderHandleWidth, Style->SliderHandleWidth);
    rect SliderHandle = rect_(Pos.x + Value, Pos.y - Style->SliderHandleHeight/2 + Style->SliderHandleWidth/2,
                              Style->SliderHandleWidth, Style->SliderHandleHeight);

    colorb HandleColor = Style->DefaultColor;

    if (IsInsideRect(Input->mPos, rect_Union(SliderGroove, SliderHandle))) {
        Ctx->Hot    = Me;
        HandleColor = Style->HotButtonColor;

        if (Input->dmWheel) {
            Value = Min(Value + (Input->dmWheel/120) * 10, Range);
        }

        if (Input->mLeftButtonIsDown) {
            Ctx->Clicked = Me;
            HandleColor  = Style->ClickedButtonColor;
            Value = Min(Input->mPos.x - Pos.x, Range);
        }
    }

    if (Value < 0)
        Value = 0;

    DrawRect(Renderer, SliderGroove, (colorb){GREY_800});
    DrawRect(Renderer, SliderHandle, HandleColor);

    Ctx->Last = Me;
    if (Ctx->Current < Ctx->NoItems)
        Ctx->Current++;

    return Value/Range;
}
#endif
#endif//UI_H



#ifndef GAP_BUFFER_H
///////////////////////////////////////////////////////////
// GAP BUFFER
#define GAP_BUFFER_H
typedef u32 cursor;
typedef struct _buffer {
    c8 *Data;
    id  Id;

    cursor GapStart;
    cursor GapEnd;
    cursor Point;
    u32    End;
} buffer;

inline u32 GetBufferGapSize(buffer *Buffer) {
    return Buffer->GapEnd - Buffer->GapStart;
}

inline u32 GetBufferLen(buffer *Buffer) {
    return Buffer->End - GetBufferGapSize(Buffer);
}

inline u32 GetCursorIndex(buffer *Buffer, cursor Cursor) {
    return (Cursor < Buffer->GapStart)? Cursor : Cursor + GetBufferGapSize(Buffer);
}

inline void AssertBufferInvariants(buffer *Buffer) {
    Assert(Buffer->Data);
    Assert(Buffer->GapStart <= Buffer->GapEnd);
    Assert(Buffer->GapEnd <= Buffer->End);
}

inline void AssertCursorInvariants(buffer *Buffer, cursor Cursor) {
    Assert(Cursor <= GetBufferLen(Buffer));
}

inline c8 GetBufferChar(buffer *Buffer, cursor Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    return Buffer->Data[GetCursorIndex(Buffer, Cursor)];
}

inline void GetBufferStr(platform_api *p, buffer *Buffer, cursor Start, cursor End, c8 *Result) {
    u32 Size = ((End - Start) >= 0)? End - Start : 0;
    for (cursor Cursor = 0; Cursor < Size; Cursor++) {
        Result[Cursor] = GetBufferChar(Buffer, Cursor + Start);
    }
    Result[Size] = 0;
}

inline void SetBufferChar(buffer *Buffer, cursor Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    Buffer->Data[GetCursorIndex(Buffer, Cursor)] = Char;
}

internal buffer *CreateBuffer(u32 InitialGapSize) {
    buffer *Buffer = GlobalPlatformApi.AllocateMemory(sizeof(buffer)); {
        Buffer->Data     = GlobalPlatformApi.AllocateMemory(InitialGapSize);
        Buffer->GapStart = 0;
        Buffer->GapEnd   = InitialGapSize;
        Buffer->End      = InitialGapSize;
        Buffer->Point    = 0;
    }

    return Buffer;
}

internal void DeleteBuffer(buffer *Buffer) {
    Buffer->GapStart = 0;
    Buffer->GapEnd   = Buffer->End;
    Buffer->Point    = 0;
}

internal void FreeBuffer(buffer *Buffer) {
    GlobalPlatformApi.FreeMemory(Buffer->Data);
    GlobalPlatformApi.FreeMemory(Buffer);
}

internal cursor MoveBufferPosFoward(buffer *Buffer, u32 Pos) {
    Assert(Pos != Buffer->End);
    Pos++;
    if (Pos == Buffer->GapStart)
        Pos =  Buffer->GapEnd;
    return Pos;
}

internal void ShiftGapToCursor(buffer *Buffer, cursor Cursor) {
    u32 GapSize = GetBufferGapSize(Buffer);
    if (Cursor < Buffer->GapStart) {
        u32 GapDelta = Buffer->GapStart - Cursor;
        Buffer->GapStart -= GapDelta;
        Buffer->GapEnd   -= GapDelta;
        MoveMemory(Buffer->Data + Buffer->GapEnd, Buffer->Data + Buffer->GapStart, GapDelta);
    }
    else
    if (Cursor > Buffer->GapStart) {
        u32 GapDelta = Cursor - Buffer->GapStart;
        MoveMemory(Buffer->Data + Buffer->GapStart, Buffer->Data + Buffer->GapEnd, GapDelta);
        Buffer->GapStart += GapDelta;
        Buffer->GapEnd   += GapDelta;
    }
    Assert(GetBufferGapSize(Buffer) == GapSize);
    AssertBufferInvariants(Buffer);
}

internal void EnsureGapSize(buffer *Buffer, u32 Min) {
    if (GetBufferGapSize(Buffer) < Min) {
        ShiftGapToCursor(Buffer, GetBufferLen(Buffer));
        u32 NewEnd = Max(2 * Buffer->End, Buffer->End + Min - GetBufferGapSize(Buffer));
        void *Temp     = GlobalPlatformApi.AllocateMemory(NewEnd);
        CopyMemory(Temp, Buffer->Data, Buffer->End);
        GlobalPlatformApi.FreeMemory(Buffer->Data);
        Buffer->Data   = Temp;
        Buffer->GapEnd = NewEnd;
        Buffer->End    = NewEnd; 
    }
    Assert(GetBufferGapSize(Buffer) >= Min);
}

internal b32 ReplaceChar(buffer *Buffer, cursor Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor < GetBufferLen(Buffer)) {
        SetBufferChar(Buffer, Cursor, Char);
        return 1;
    }
    else {
        return 0;
    }
}

internal void InsertChar(buffer *Buffer, cursor Cursor, c8 Char) {
    AssertCursorInvariants(Buffer, Cursor);
    EnsureGapSize(Buffer, 1);
    ShiftGapToCursor(Buffer, Cursor);
    Buffer->Data[Buffer->GapStart] = Char;
    Buffer->GapStart++;
    if (Buffer->Point >= Cursor) Buffer->Point++;
}

internal b32 DeleteBackwardChar(buffer *Buffer, cursor Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor > 0) {
        ShiftGapToCursor(Buffer, Cursor);
        Buffer->GapStart--;
        if (Buffer->Point >= Cursor) Buffer->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal b32 DeleteFowardChar(buffer *Buffer, cursor Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    if (Cursor < GetBufferLen(Buffer)) {
        ShiftGapToCursor(Buffer, Cursor);
        Buffer->GapEnd++;
        if (Buffer->Point > Cursor) Buffer->Point--;
        return 1;
    }
    else {
        return 0;
    }
}

internal void SaveBuffer(buffer *Buffer, c8 *Filename) {
    GlobalPlatformApi.WriteFile(Buffer->Data, Buffer->GapStart, Filename, 0);
    GlobalPlatformApi.WriteFile(Buffer->Data + Buffer->GapEnd, Buffer->End - Buffer->GapEnd, Filename, 1);
}

internal void LoadBuffer(buffer *Buffer, c8 *Filename) {
    if (Filename) {
        file File = GlobalPlatformApi.LoadFile(Filename);
        DeleteBuffer(Buffer);
        EnsureGapSize(Buffer, File.Size);
        //todo: unecessary CopyMemory?
        CopyMemory(Buffer->Data, File.Data, File.Size);
        Buffer->GapStart = File.Size;
        GlobalPlatformApi.FreeFile(File);
    }
}

inline cursor GetNextCharCursor(buffer *Buffer, cursor Cursor) {
    if (Cursor < GetBufferLen(Buffer)) {
        Cursor++;
        if (GetBufferChar(Buffer, Cursor) == '\r')
            Cursor++;
    }
    return Cursor;
}

inline cursor GetPrevCharCursor(buffer *Buffer, cursor Cursor) {
    if (Cursor > 0) {
        Cursor--;
        if (GetBufferChar(Buffer, Cursor) == '\r')
            Cursor--;
    }
    return Cursor;
}

inline cursor GetBeginningOfLineCursor(buffer *Buffer, cursor Cursor) {
    AssertCursorInvariants(Buffer, Cursor);
    Cursor = GetPrevCharCursor(Buffer, Cursor);
    while (Cursor > 0) {
        c8  Char = GetBufferChar(Buffer, Cursor);
        if (Char == '\n') {
            return GetNextCharCursor(Buffer, Cursor);
        }
        Cursor = GetPrevCharCursor(Buffer, Cursor);
    }
    return 0;
}

inline cursor GetEndOfLineCursor(buffer *Buffer, cursor CurrentCursor) {
    AssertCursorInvariants(Buffer, CurrentCursor);
    while (CurrentCursor < GetBufferLen(Buffer)) {
        c8  Char = GetBufferChar(Buffer, CurrentCursor);
        if (Char == '\n') {
            return CurrentCursor;
        }
        CurrentCursor = GetNextCharCursor(Buffer, CurrentCursor);
    }
    return GetBufferLen(Buffer);
}

inline cursor GetBeginningOfNextLineCursor(buffer *Buffer, cursor CurrentCursor) {
    return GetNextCharCursor(Buffer, GetEndOfLineCursor(Buffer, CurrentCursor));
}

inline cursor GetEndOfPrevLineCursor(buffer *Buffer, cursor CurrentCursor) {
    return GetPrevCharCursor(Buffer, GetBeginningOfLineCursor(Buffer, CurrentCursor));
}

inline cursor GetBeginningOfPrevLineCursor(buffer *Buffer, cursor CurrentCursor) {
    return GetBeginningOfLineCursor(Buffer, GetPrevCharCursor(Buffer, GetBeginningOfLineCursor(Buffer, CurrentCursor)));
}

inline cursor GetCursorColumn(buffer *Buffer, cursor Cursor) {
    return Cursor - GetBeginningOfLineCursor(Buffer, Cursor);
}

inline cursor GetLineLen(buffer *Buffer, cursor Cursor) {
    return GetEndOfLineCursor(Buffer, Cursor) - GetBeginningOfLineCursor(Buffer, Cursor);
}

inline cursor GetBegginingOfBufferCursor(buffer *Buffer, cursor CurrentCursor) {
    return 0;
}

inline cursor GetEndOfBufferCursor(buffer *Buffer, cursor CurrentCursor) {
    return GetBufferLen(Buffer);
}

inline cursor GetBufferColumn(buffer *Buffer, cursor CurrentCursor) {
    return CurrentCursor - GetBeginningOfLineCursor(Buffer, CurrentCursor);
}

inline cursor GetBufferLine(buffer *Buffer, cursor CurrentCursor) {
    u32 Line = 0;
    do {
        CurrentCursor = GetBeginningOfPrevLineCursor(Buffer, CurrentCursor);
        Line++;
    } while (GetBeginningOfPrevLineCursor(Buffer, CurrentCursor) != 0);
    return Line;
}
#endif//GAP_BUFFER_H



#ifndef STRETCHY_BUFFER_H
///////////////////////////////////////////////////////////
//// STRETCHY_BUFFER_H
#define STRETCHY_BUFFER_H
#include "lingo.h"
//note: will this ever be used?
//note:
//  Stretchy buffers.
//  Invented by Sean Barret?
//  Interface:
//    i32 *Buffer = NULL;
//    BufferPush(Buffer, 42);
//    BufferPush(Buffer, 21);
//    for (i32 i = 0, i < BufferLen(Buffer), i++) {...}
//

typedef struct _buffer_header {
    u32 Len;
    u32 Capacity;
    c8 *Buffer;
} buffer_header;

// annotation
#define BUFF(x) x

/* internal functions
    */
#define _BufferHeader(Buffer) ((buffer_header *)((c8 *)Buffer - offsetof(buffer_header, Buffer)))
#define _BufferFits  (Buffer, n)    (GetBuffeLen(Buffer) + (n) <= GetBufferCapacity(Buffer))
#define _BufferFit   (Buffer, n, p)(_BufferFits(Buffer, n) ? 0 : ((Buffer) = _GrowBuffer(p, (Buffer), GetBuffeLen(Buffer) + (n), sizeof(*(Buffer)))))

/* public functions
    */
#define GetBuffeLen (Buffer     ) ((Buffer) ? _BufferHeader(Buffer)->Len      : 0)
#define GetBufferCapacity(Buffer) ((Buffer) ? _BufferHeader(Buffer)->Capacity : 0)
#define GetBufferEnd(Buffer     ) ((Buffer) +  GetBuffeLen (Buffer))
#define FreeBuffer  (Buffer, p  ) ((Buffer) ? (p->FreeMemory(_BufferHeader(Buffer)), (Buffer) = NULL) : 0)
#define PrintfBuffer(Buffer, ...) ((Buffer) = _PrintfBuffer((Buffer), __VA_ARGS__))
#define ClearBuffer (Buffer     ) ((Buffer) ? _BufferHeader (Buffer)->Len = 0 : 0)
#define PushToBuffer(Buffer, ...) (_BufferFit((Buffer), 1), (Buffer)[_BufferHeader(Buffer)->Len++] = (__VA_ARGS__))

internal void *_GrowBuffer(platform_api *p, const c8 *Buffer, u32 NewLen, u32 ElementSize) {
    Assert(GetBufferCapacity(Buffer) <= (SIZE_MAX - 1)/2);
    u32 NewCapacity = Max(1 + 2 * GetBufferCapacity(Buffer), NewLen);
    Assert(NewCapacity >= NewLen);
    Assert(NewCapacity <= (SIZE_MAX - offsetof(buffer_header, Buffer))/ElementSize);
    u32 NewSize = offsetof(buffer_header, Buffer) + NewCapacity * ElementSize;

    buffer_header *NewHeader;
    if (Buffer) {
        p->FreeMemory(_BufferHeader(Buffer));
        NewHeader = p->AllocateMemory(NewSize);
    }
    else {
        NewHeader      = p->AllocateMemory(NewSize);
        NewHeader->Len = 0;
    }
    NewHeader->Capacity = NewCapacity;
    return NewHeader->Buffer;
}

internal c8 *_PrintfBuffer(c8 *Buffer, const c8 *Format, ...) {
    va_list Args;
    va_start(Args, Format);
    u32 n = vsnprintf(NULL, 0, Format, Args);
    va_end(Args);
    if (GetBuffeLen(Buffer) == 0) {
        n++;
    }
    _BufferFit(Buffer, n + GetBuffeLen(Buffer));
    c8 *Dest = GetBuffeLen(Buffer) == 0 ? Buffer : Buffer + GetBuffeLen(Buffer) - 1;
    va_start(Args, Format);
    vsnprintf(Dest, Buffer + GetBufferCapacity(Buffer) - Dest, Format, Args);
    va_end(Args);
    _BufferHeader(Buffer)->Len += n;
    return Buffer;
}
#endif//STRETCHY_BUFFER_H



#ifndef LEXER_H
///////////////////////////////////////////////////////////
// LEXER
#define LEXER_H
const global u8 CHAR_TO_DIGIT[128] = {
    ['0'] = 0,
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['a'] = 10,
    ['A'] = 10,
    ['b'] = 11,
    ['B'] = 11,
    ['c'] = 12,
    ['C'] = 12,
    ['d'] = 13,
    ['D'] = 13,
    ['e'] = 14,
    ['E'] = 14,
    ['f'] = 15,
    ['F'] = 15,
};

const global c8 ESCAPE_TO_CHAR[128] = {
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['b'] = '\b',
    ['a'] = '\a',
    ['0'] =   0,
};

//note: how?

global c8 *StructKeyword = "struct";
global c8 *UnionKeyword  = "union";
global c8 *EnumKeyword   = "enum";
global c8 *IfKeyword     = "if";
global c8 *ElseKeyword   = "else";
global c8 *WhileKeyword  = "while";
global c8 *ForKeyword    = "for";
global c8 *FirstKeyword = StructKeyword;
global c8 *LastKeyword  = ForKeyword;

typedef struct _lex_name_table {
    c8 *Names[128];
    u32 UsedNames;
} lex_name_table;

global lex_name_table Names;

internal void PushName(c8 *Name) {
    if (Names.UsedNames < 128) {
        if (ArrayCount(Name) < 64)
            Names[Names.UsedNames] = Name;
    }
    else {
        Names.UsedNames = 0;
    }
}

internal void SetKeywords() {
    PushName(StructKeyword);
    PushName(UnionKeyword);
    PushName(EnumKeyword);
    PushName(IfKeyword);
    PushName(ElseKeyword);
    PushName(WhileKeyword);
    PushName(ForKeyword);
}

typedef enum _token_type {
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_DELIMITER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_STR,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_NAME,
    TOKEN_TYPE_COMMENT
} token_type;

typedef enum _token_mode {
    TOKEN_MODE_NONE,
    TOKEN_MODE_HEX,
    TOKEN_MODE_OCT,
    TOKEN_MODE_BIN,
    TOKEN_MODE_CHAR
} token_mode;

typedef struct _token {
    token_type Type;
    token_mode Mode;

    u32 Error;

    cursor Start;
    cursor End;

    union {
        u64 iVal;
        f64 fVal;
        c8 *sVal;
        c8  Name[256];
    };
} token;

internal cursor ScanInt(buffer *Buffer, cursor Cursor, token *Token) {
    Token->Start = Cursor;
    u64 Base = 10;
    if (GetBufferChar(Buffer, Cursor) == '0') {
        Cursor++;
        if (tolower(GetBufferChar(Buffer, Cursor)) == 'x') {
            Cursor++;
            Token->Mode = TOKEN_MODE_HEX;
            Base = 16;
        }
        else
        if (tolower(GetBufferChar(Buffer, Cursor)) == 'b') {
            Cursor++;
            Token->Mode = TOKEN_MODE_BIN;
            Base = 2;
        }
        else
        if (isdigit(GetBufferChar(Buffer, Cursor))) {
            Token->Mode = TOKEN_MODE_OCT;
            Base = 8;
        }
    }
    u64 Val = 0;
    while (1) {
        u64 Digit = CHAR_TO_DIGIT[GetBufferChar(Buffer, Cursor)];
        if (Digit == 0 && GetBufferChar(Buffer, Cursor) != '0') {
            break;
        }
        if (Digit > Base) {
            //todo: switch color to some error color or use those squiggly undelines idk
            Token->Error = 1;
        }
        if (Val > (UINT64_MAX - Digit)/Base) {
            //note: see above todo
            Token->Error = 1;
            while (isdigit(GetBufferChar(Buffer, Cursor)))
                Cursor++;
            Val = 0;
        }
        Val = Val * Base + Digit;
        Cursor++;
    }
    Token->End  = Cursor;
    Token->iVal = Val;
    Token->Type = TOKEN_TYPE_INT;

    return Cursor;
}

internal cursor ScanFloat(buffer *Buffer, cursor Cursor, token *Token) {
    cursor Start = Cursor;
    Token->Start = Start;
    while (isdigit(GetBufferChar(Buffer, Cursor))) {
        Cursor++;
    }
    if (GetBufferChar(Buffer, Cursor) == '.') {
        Cursor++;
    }
    Cursor++;
    while (isdigit(GetBufferChar(Buffer, Cursor))) {
        Cursor++;
    }
    if (tolower(GetBufferChar(Buffer, Cursor)) == 'e') {
        Cursor++;
        if (GetBufferChar(Buffer, Cursor) == '+' || GetBufferChar(Buffer, Cursor) == '-') {
            Cursor++;
        }
        if (!isdigit(GetBufferChar(Buffer, Cursor))) {
            Token->Error = 1;
        }
        while (isdigit(GetBufferChar(Buffer, Cursor))) {
            Cursor++;
        }
    }
    // c8 *End = Cursor;
    f64 Val = 0;//strtod(Start, NULL); todo
    if (Val == HUGE_VAL || Val == -HUGE_VAL) {
        Token->Error = 1;
    }
    Token->End  = Cursor;
    Token->fVal = Val;
    Token->Type = TOKEN_TYPE_FLOAT;

    return Cursor;
}

internal cursor ScanStr(buffer *Buffer, cursor Cursor, token *Token) {
    Assert(GetBufferChar(Buffer, Cursor) == '"');
    Token->Start = Cursor;
    Cursor++;
    char *Str = NULL;
    while (GetBufferChar(Buffer, Cursor) && GetBufferChar(Buffer, Cursor) != '"') {
        char Val = GetBufferChar(Buffer, Cursor);
        if (Val == '\n') {
            Token->Error = 1;
        }
        else
        if (Val == '\\') {
            Cursor++;
            Val = ESCAPE_TO_CHAR[GetBufferChar(Buffer, Cursor)];
            if (Val == 0 && GetBufferChar(Buffer, Cursor) != '0') {
                Token->Error = 1;
            }
        }
        Cursor++;
    }
    if (GetBufferChar(Buffer, Cursor)) {
        Assert(GetBufferChar(Buffer, Cursor) == '"');
        Cursor++;
    }
    else {
        Token->Error = 1;
    }
    Token->End  = Cursor;
    Token->sVal = Str;
    Token->Type = TOKEN_TYPE_STR;

    return Cursor;
}

internal cursor ScanChar(buffer *Buffer, cursor Cursor, token *Token) {
    Token->Start = Cursor;
    Cursor++;
    c8 Val = 0;
    if (GetBufferChar(Buffer, Cursor) == '\'') {
        //
        Cursor++;
    }
    else
    if (GetBufferChar(Buffer, Cursor) == '\n') {
        //
    }
    else
    if (GetBufferChar(Buffer, Cursor) == '\\') {
        Cursor++;
        Val = ESCAPE_TO_CHAR[GetBufferChar(Buffer, Cursor)];
        if (Val == 0 && GetBufferChar(Buffer, Cursor) != '0') {
            //
        }
        Cursor++;
    }
    else {
        Val = GetBufferChar(Buffer, Cursor);
        Cursor++;
    }
    
    if (GetBufferChar(Buffer, Cursor) != '\'') {
        //
    }
    else {
        Cursor++;
    }
    Token->End  = Cursor;
    Token->iVal = Val;
    Token->Type = TOKEN_TYPE_INT;
    Token->Mode = TOKEN_MODE_CHAR;

    return Cursor;
}

#define CASE1(c, c1, k)                                 \
    case c: {                                           \
        Token->Type = GetBufferChar(Buffer, Cursor)++;  \
        if (GetBufferChar(Buffer, Cursor) == c1) {      \
            Token->Type = k;                            \
            Cursor++;                                   \
        }                                               \
        break;                                          \
    }

#define CASE2(c, c1, k, c2, k1)                         \
    case c: {                                           \
        Token->Type = GetBufferChar(Buffer, Cursor)++;  \
        if (GetBufferChar(Buffer, Cursor) == c1) {      \
            Token->Type = k;                            \
            Cursor++;                                   \
        }                                               \
        else                                            \
        if (GetBufferChar(Buffer, Cursor) == c2) {      \
            Token->Type = k1;                           \
            Cursor++;                                   \
        }                                               \
        break;                                          \
    }

internal c8 *GetSubStr(c8 *Start, c8 *End) {
    u32 Len = (i32)(End - Start);
    c8 *Str = GlobalPlatformApi.AllocateMemory(Len + 1);
    CopyMemory(Str, Start, Len);
    Str[Len] = 0;
    return Str;
}

b32 IsKeyword(c8 *Name) {
    //todo: push all names to a global table and compare indicies
    b32 Result = 0;
    if (FirstKeyword < Name && Name < LastKeyword)
        Result = 1;
    return Result;
}

internal cursor NextToken(platform_api *p, buffer *Buffer, cursor Cursor, token *Token) {
TOP:
    p->FreeMemory(Token->Name);
    Token->Start = Cursor;
    Token->Mode  = 0;
    switch (GetBufferChar(Buffer, Cursor)) {
        case '\r': case '\v': {
            while (GetBufferChar(Buffer, Cursor) == '\r' || GetBufferChar(Buffer, Cursor) == '\v') {
                Cursor++;
            }
            goto TOP;
            break;
        }
        case '\'': {
            Cursor = ScanChar(Buffer, Cursor, Token);
            break;
        }
        case '"': {
            Cursor = ScanStr(Buffer, Cursor, Token);
            break;
        }
        case '/': {
            Cursor++;
            if (GetBufferChar(Buffer, Cursor) == '/') {
                while (GetBufferChar(Buffer, Cursor) != '\n')
                    Cursor++;
                Token->Type = TOKEN_TYPE_COMMENT;
            }
            else
            if (GetBufferChar(Buffer, Cursor) == '*') {
                Cursor++;
                while (GetBufferChar(Buffer, Cursor) != '*')
                    Cursor++;
                if (GetBufferChar(Buffer, Cursor) == '*') {
                    Cursor++;
                    if (GetBufferChar(Buffer, Cursor) == '/')
                        Token->Type = TOKEN_TYPE_COMMENT;
                }
                else {
                    Token->Error = 1;
                }
            }
            break;
        }
        case '.': {
            if (isdigit(GetBufferChar(Buffer, Cursor) + 1)) {
                Cursor = ScanFloat(Buffer, Cursor, Token);
            }
            else {
                Cursor++;
                Token->Type = TOKEN_TYPE_OPERATOR;
            }

            break;
        }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            while (isdigit(GetBufferChar(Buffer, Cursor))) {
                Cursor++;
            }
            c8 c = GetBufferChar(Buffer, Cursor);
            Cursor = Token->Start;
            if (c == '.' || tolower(c) == 'e') {
                Cursor = ScanFloat(Buffer, Cursor, Token);
            }
            else {
                Cursor = ScanInt(Buffer, Cursor, Token);
            }
            break;
        }
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y':
        case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y':
        case 'Z':
        case '_': {
            while (isalnum(GetBufferChar(Buffer, Cursor)) || GetBufferChar(Buffer, Cursor) == '_') {
                Cursor++;
            }
            GetBufferStr(p, Buffer, Token->Start, Cursor, Token->Name);
            PushName(Token->Name);
            Token->Type = IsKeyword(Token->Name)? TOKEN_TYPE_KEYWORD : TOKEN_TYPE_NAME;
            break;
        }
        case '|':
        case '^':
        case '*':
        case '%':
        case '=':
        case '!':
        case '+':
        case '-':
        case '<':
        case '>':
        case '?':
        case ':':
        case ',': {
            Token->Type = TOKEN_TYPE_OPERATOR;
            Cursor++;
            break;
        }
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case ';': {
            Token->Type = TOKEN_TYPE_DELIMITER;
            Cursor++;
            break;
        }
        case '#': {
            Cursor++;
            while (isalnum(GetBufferChar(Buffer, Cursor)) || GetBufferChar(Buffer, Cursor) == '_') {
                Cursor++;
            }
            Token->Type = TOKEN_TYPE_KEYWORD;
            break;
        }
        default: {
            Token->Type = GetBufferChar(Buffer, Cursor++);
            break;
        }
    }
    Token->End = Cursor;

    return Cursor;
}

inline cursor GetNextTokenCursor(buffer *Buffer, cursor Cursor) {
    cursor Result = Cursor;
    u32    BufferLen = GetBufferLen(Buffer);
    if (GetBufferChar(Buffer, Result) == ' ')
        Result++;
    while (GetBufferChar(Buffer, Result) != ' ' && Result < BufferLen) {
        Result++;
        if (GetBufferChar(Buffer, Result) == '\n')
            break;
    }

    return Result;
}

inline cursor GetPrevTokenCursor(buffer *Buffer, cursor Cursor) {
    cursor Result = Cursor;
    u32    BufferLen = GetBufferLen(Buffer);
    if (GetBufferChar(Buffer, Result) == ' ')
        Result--;
    while (GetBufferChar(Buffer, Result) != ' ' && Result > 0) {
        Result--;
        if (GetBufferChar(Buffer, Result) == '\n')
            break;
    }

    return Result;
}
#endif//LEXER_H



#ifndef COMMANDS_H
///////////////////////////////////////////////////////////
// COMMANDS
#define COMMANDS_H
typedef struct _editor_context {
    renderer *Renderer;

    rv2 mPos;
    r32 dtFrame;
    u16 LastKeyComb;
    c8  LastChar;

    // ui_style *uiStyle;
    // ui_ctx   *uiCtx;
    // ui_input *uiInput;

    id  CurrentBuffer;
    buffer **Buffers;
    u32 CurrentBufferLine;
    u32 CurrentBufferColumn;

    // rv2 CurrentCaretPos;
} editor_context;

#define CMD_PROC(Name) void cmd_proc_##Name(editor_context *c)
typedef CMD_PROC(callback);

CMD_PROC(DoNothing) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    //note: does nothing for now, consider error message.
}

CMD_PROC(InsertChar) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point,
               c->LastChar);
}

CMD_PROC(DeleteCharFoward) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    DeleteFowardChar(Buffer, Buffer->Point);
}

CMD_PROC(DeleteCharBackward) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    DeleteBackwardChar(Buffer, Buffer->Point);
}

CMD_PROC(Indent) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point, '\t');
}

CMD_PROC(MoveCarretLeft) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetPrevCharCursor(Buffer, Buffer->Point);
    c->CurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretRight) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetNextCharCursor(Buffer, Buffer->Point);
    c->CurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToPrevToken) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetPrevTokenCursor(Buffer, Buffer->Point);
    c->CurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToNextToken) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetNextTokenCursor(Buffer, Buffer->Point);
    c->CurrentBufferColumn = GetBufferColumn(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretUp) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    u32 BeginningOfPrevLine = GetBeginningOfPrevLineCursor(Buffer, Buffer->Point);
    u32 PrevLineLen         = GetLineLen(Buffer, BeginningOfPrevLine);
    Buffer->Point = BeginningOfPrevLine + Min(PrevLineLen, c->CurrentBufferColumn);
    c->CurrentBufferLine = GetBufferLine(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretDown) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    u32 BeginningOfNextLine = GetBeginningOfNextLineCursor(Buffer, Buffer->Point);
    u32 NextLineLen         = GetLineLen(Buffer, BeginningOfNextLine);
    Buffer->Point = Min(BeginningOfNextLine + Min(NextLineLen, c->CurrentBufferColumn), GetBufferLen(Buffer) - 1);
    c->CurrentBufferLine = GetBufferLine(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToLineStart) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point =
        GetBeginningOfLineCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToLineEnd) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetEndOfLineCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToBufferStart) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetBegginingOfBufferCursor(Buffer, Buffer->Point);
}

CMD_PROC(MoveCarretToBufferEnd) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    Buffer->Point = GetEndOfBufferCursor(Buffer, Buffer->Point);
}

CMD_PROC(InsertNewLine) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    InsertChar(Buffer, Buffer->Point, '\n');
}

CMD_PROC(SaveFile) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    // SaveBuffer(Buffer, c->Filename);
}

CMD_PROC(OpenFile) {
    buffer *Buffer = c->Buffers[c->CurrentBuffer];
    // LoadBuffer(Buffer, c->Filename);
}

typedef struct _command {
    cmd_proc_callback *Proc;
    c8                *Desc;
} command;
#define command_(Proc, Desc) (command){(Proc), (Desc)}

typedef enum _key {
    KEY_NONE = 0, //note:
    KEY_CHAR = 1, //note:
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
    KEY_CTRL  = 1 << 8,
    KEY_ALT   = 1 << 9,
    KEY_SHIFT = 1 << 10
} key;

#define KeyComb(BaseKey, Ctrl, Alt, Shift) (u16)(BaseKey) | ((u16)(Ctrl) << 8) | ((u16)(Alt)  << 9) | ((u16)(Shift) << 10)
#define Ctrl(Key)  KeyComb(Key, 1, 0, 0)
#define Alt(Key)   KeyComb(Key, 0, 1, 0)
#define Shift(Key) KeyComb(Key, 0, 0, 1)

/*internal void UpdateEditorContextInput(editor_context *c, platform *p) {
    c->uiInput->mPos = p->mPos;
    c->uiInput->mLeft = p->mLeft;
    c->dtFrame = p->dtForFrame;
    
    u16 Key = KEY_NONE;
    if (p->kDelete)
        Key = KeyComb(KEY_DEL, p->kCtrl, p->kAlt, p->kShift);
    if (p->kBack)
        Key = KeyComb(KEY_BACK, p->kCtrl, p->kAlt, p->kShift);
    if (p->kTab)
        Key = KeyComb(KEY_TAB, p->kCtrl, p->kAlt, p->kShift);
    if (p->kLeft)
        Key = KeyComb(KEY_LEFT, p->kCtrl, p->kAlt, p->kShift);
    if (p->kRight)
        Key = KeyComb(KEY_RIGHT, p->kCtrl, p->kAlt, p->kShift);
    if (p->kUp)
        Key = KeyComb(KEY_UP, p->kCtrl, p->kAlt, p->kShift);
    if (p->kDown)
        Key = KeyComb(KEY_DOWN, p->kCtrl, p->kAlt, p->kShift);
    if (p->kHome)
        Key = KeyComb(KEY_HOME, p->kCtrl, p->kAlt, p->kShift);
    if (p->kPgUp)
        Key = KeyComb(KEY_PG_UP, p->kCtrl, p->kAlt, p->kShift);
    if (p->kPgDown)
        Key = KeyComb(KEY_PG_DOWN, p->kCtrl, p->kAlt, p->kShift);
    if (p->kEnd)
        Key = KeyComb(KEY_END, p->kCtrl, p->kAlt, p->kShift);
    if (p->kReturn)
        Key = KeyComb(KEY_RETURN, p->kCtrl, p->kAlt, p->kShift);
    if (p->kChar && IsPrintableChar(p->Char)) {
        if (!p->kCtrl)
            Key = KEY_CHAR;
        else
            Key = KeyComb(p->Char, p->kCtrl, p->kAlt, p->kShift);
    }
    c->LastChar    = p->Char;
    c->LastKeyComb = Key;
}*/
#endif//COMMANDS_H
#endif//APP_H