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

    if (IsKeyDown(KEY_W) && (GameState->Rect.y > 0))
        GameState->Rect.y -= GetFrameTime()*750.0f;
    if (IsKeyDown(KEY_S) && (GameState->Rect.y < GameState->ScreenHeight))
        GameState->Rect.y += GetFrameTime()*750.0f;
    if (IsKeyDown(KEY_A) && (GameState->Rect.x > 0))
        GameState->Rect.x -= GetFrameTime()*750.0f;
    if (IsKeyDown(KEY_D) && (GameState->Rect.x < GameState->ScreenWidth))
        GameState->Rect.x += GetFrameTime()*750.0f;

    DrawRectanglePro(GameState->Rect, { GameState->Rect.width/2, GameState->Rect.height/2 }, 0.0f, BLUE);
    DrawCircle(GameState->ScreenWidth/2, GameState->ScreenHeight/2, 100.0f, GREEN);
}
