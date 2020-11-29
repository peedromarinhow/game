#include "raylib.h"
#include "win32_main.h"
#include "game.h"
#include "Windows.h"

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
    assert(sizeof(game_state) <= GameMem->PermaStorageSize);
    game_state *GameState = (game_state *)GameMem->PermaStorageBytes;

    if(!GameMem->IsInitialized) {
        GameState->Rect = { 100, 100, 100, 100 };
        GameState->ScreenHeight = GetScreenHeight();
        GameState->ScreenWidth  = GetScreenWidth();

        char* Filename = __FILE__;
        debug_read_file_result File = GameMem->DEBUGPlatformReadWholeFile(Filename);
        if (File.Contents) {
            GameMem->DEBUGPlatformWriteWholeFile("test.out", File.Contents, SafeTruncateUint64(File.ContentsSize));
            GameMem->DEBUGPlatformFreeWholeFile(File.Contents);
        } else {
            //
        }

        GameMem->IsInitialized = true;
    }

    DrawRectanglePro(GameState->Rect, { GameState->Rect.width/2, GameState->Rect.height/2 }, 0.0f, BLUE);
    DrawCircle(GameState->ScreenWidth/2, GameState->ScreenHeight/2, 100.0f, YELLOW);
}
