#include <windows.h>
#include <gl/gl.h>

#include "ft2build.h"
#include "freetype/freetype.h"
#include "lingo.h"
#include "maths.h"
#include "platform.h"
#include "memory.h"

typedef struct _app_state {
    rv2 PlayerPos;
    memory_arena FreeTypeTestArena;
} app_state;

typedef struct __freetype_test {
    FT_Library Library;
    FT_Face    Face;
} _freetype_test;

__declspec(dllexport) APP_INIT(Init) {
    Assert(sizeof(app_state) <= Plat->Memory.Size);
    app_state *State = (app_state *)Plat->Memory.Contents;
    State->PlayerPos = (rv2){100, 100};
    InitializeArena(&State->FreeTypeTestArena, Plat->Memory.Size - sizeof(app_state), (u8 *)Plat->Memory.Size + sizeof(app_state));
    _freetype_test *FreetypeTest = PushStructToArena(&State->FreeTypeTestArena, _freetype_test);
    if (FT_Init_FreeType(&FreetypeTest->Library)) {
        Assert(!"NOOOO!!");
    }
    if (FT_New_Face(FreetypeTest->Library, "d:/fontes/IM_Fell_French_Canon/IMFellFrenchCanon-Regular.ttf", 0, &FreetypeTest->Face)) {
        Assert(!"NOOOO!!");
    }
}

__declspec(dllexport) APP_UPDATE(Update) {
    app_state *State = (app_state *)Plat->Memory.Contents;

    // glViewport(0, 0, Plat->WindowSize.Width, Plat->WindowSize.Height);

    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();

    // glMatrixMode(GL_PROJECTION);
    // glLoadIdentity();

    // r32 a = 2.0f/Plat->WindowSize.Width;
    // r32 b = 2.0f/Plat->WindowSize.Height;
    // r32 Proj[] = {
    //      a,  0,  0,  0,
    //      0, -b,  0,  0,
    //      0,  0,  1,  0,
    //     -1,  1,  0,  1
    // };
    // glLoadMatrixf(Proj);

    _freetype_test *FreetypeTest = (_freetype_test *)State->FreeTypeTestArena.Base;

    if (FT_Set_Char_Size(FreetypeTest->Face, 0, 16*64, 71, 72)) {
        Assert(!"NOOOO!!");
    }
    u32 GlyphIndex = FT_Get_Char_Index(FreetypeTest->Face, 'a');
    if (FT_Load_Glyph(FreetypeTest->Face, GlyphIndex, 0)) {
        Assert(!"NOOOO!!");
    }
    if (FT_Render_Glyph(FreetypeTest->Face->glyph, FT_RENDER_MODE_NORMAL)) {
        Assert(!"NOOOO!!");
    }
}
