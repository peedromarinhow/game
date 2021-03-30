#include "lingo.h"
#include "app.h"

#include "maths.h"
#include "platform.h"
#include "memory.h"
#include "opengl.h"

#include "graphics.h"
#include "fonts.h"

typedef struct _app_state {
    font   Font;
    texture A;
} app_state;

external APP_INIT(Init) {
    Assert(sizeof(app_state) <= p->Memory.Size);
    app_state *State = (app_state *)p->Memory.Contents;

    AllocateMemory    = p->AllocateMemoryCallback;
    FreeMemory        = p->FreeMemoryCallback;
    LoadFile          = p->LoadFileCallback;
    FreeFile          = p->FreeFileCallback;
    LoadFileToArena   = p->LoadFileToArenaCallback;
    FreeFileFromArena = p->FreeFileFromArenaCallback;
    WriteFile_        = p->WriteFileCallback;
    ReportError       = p->ReportErrorCallback;
    ReportErrorAndDie = p->ReportErrorAndDieCallback;

    FT_Library   FreeTypeLib;
    FT_Init_FreeType(&FreeTypeLib);
    FT_Face      face;
    FT_New_Face(FreeTypeLib, "D:\\code\\platform-layer\\data\\eb_garamond.ttf", 0, &face);
    FT_Set_Pixel_Sizes(
          face,   /* handle to face object */
          0,      /* pixel_width           */
          16 );   /* pixel_height          */
    FT_Load_Char(face, 207, FT_LOAD_RENDER);
    FT_GlyphSlot slot = face->glyph;  /* a small shortcut */
    FT_Bitmap bmp;
    FT_Bitmap_Init(&bmp);
    FT_Bitmap_Convert(FreeTypeLib, &slot->bitmap, &bmp, 4);
    State->A.w = slot->metrics.width;
    State->A.h = slot->metrics.height;
    glGenTextures(1, &State->A.Id);
    glBindTexture(GL_TEXTURE_2D, State->A.Id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, slot->bitmap_left, slot->bitmap_top, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
}

external APP_RELOAD(Reload) {
    app_state *State = (app_state *)p->Memory.Contents;

    AllocateMemory    = p->AllocateMemoryCallback;
    FreeMemory        = p->FreeMemoryCallback;
    LoadFile          = p->LoadFileCallback;
    FreeFile          = p->FreeFileCallback;
    LoadFileToArena   = p->LoadFileToArenaCallback;
    FreeFileFromArena = p->FreeFileFromArenaCallback;
    WriteFile_        = p->WriteFileCallback;
    ReportError       = p->ReportErrorCallback;
    ReportErrorAndDie = p->ReportErrorAndDieCallback;
}

external APP_UPDATE(Update) {
    app_state *State = (app_state *)p->Memory.Contents;
    gBegin(Rv2(0, 0), p->WindowDimensions, Color4f(0.2f, 0.2f, 0.2f, 1));

    gDrawTexture(State->A, Rv2(p->WindowDimensions.w/2, p->WindowDimensions.h/2),
                           Rv2(State->A.w, State->A.h), Color4f(0, 0, 0, 0));

    // gDrawTexture(State->Font.Atlas, Rv2(p->WindowDimensions.w/2, p->WindowDimensions.h/2),
    //                                 Rv2(State->Font.Atlas.w, State->Font.Atlas.h),
    //                                 Color4f(0, 0, 0, 0));
}

external APP_DEINIT(Deinit) {
    app_state *State = (app_state *)p->Memory.Contents;
}