/*
 *  #ERROR# : instantaneous dynamic live code loading is not working
 *            maybe because of some fail in CopyFile
 */

#include "raylib.h"
#include "win32_main.h"
#include "game.h"
#include "Windows.h"

DEBUG_PLATFORM_FREE_WHOLE_FILE(DEBUGPlatformFreeWholeFile) {
    if (Mem) {
        VirtualFree(Mem, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_WHOLE_FILE(DEBUGPlatformReadWholeFile) {
    debug_read_file_result Res = {0};
    HANDLE FileHandle = CreateFileA(
        Filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (FileHandle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize)) {
            Res.Contents = VirtualAlloc(0, (size_t)FileSize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            uint32 FileSize32 = SafeTruncateUint64(FileSize.QuadPart);
            if (Res.Contents) {
                DWORD bytesRead;
                if (ReadFile(FileHandle, Res.Contents, FileSize32, &bytesRead, 0) && (FileSize32 == bytesRead)) {
                    // sucess
                    Res.ContentsSize = FileSize32;
                } else {
                    // TODO: logging
                    //          DEBUGplatformFreeWholeFile(Res.Contents);
                    Res.Contents = 0;
                }
            } else {
                // TODO: logging
            }
        } else {
            // TODO: logging
        }

        CloseHandle(FileHandle);
    } else {
        // TODO: logging
    }

    return Res;
}

DEBUG_PLATFORM_WRITE_WHOLE_FILE(DEBUGPlatformWriteWholeFile) {
    bool Res = false;
    HANDLE FileHandle = CreateFileA(
        Filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        0,
        NULL
    );

    if (FileHandle != INVALID_HANDLE_VALUE) {
        DWORD BytesWritten;
        if (WriteFile(FileHandle, Mem, MemSize, &BytesWritten, 0)) {
            // sucess
            Res = (MemSize == BytesWritten);
        } else {
            // TODO: logging
        }
        CloseHandle(FileHandle);
    } else {
        // TODO: logging
    }

    return Res;
}

typedef struct _win32_game_code {
    HMODULE CodeDLL;
    FILETIME LastDLLWriteTime;
    game_update_and_render *UpdateAndRender;
        // ... other functions
    bool IsValid;
} win32_game_code;

inline FILETIME Win32GetLastFileWriteTime(char *FileName) {
    FILETIME LastWriteTime = {0};
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle = FindFirstFileA(FileName, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE) {
        LastWriteTime = FindData.ftLastWriteTime;
        FindClose(FindHandle);
    }
    return LastWriteTime;
}

inline FILETIME Win32GetFileLastWriteTime(char *FileName) {
  FILETIME Result = {};
  WIN32_FILE_ATTRIBUTE_DATA Data;

  if(GetFileAttributesExA(FileName, GetFileExInfoStandard, &Data)) {
    Result = Data.ftLastWriteTime;
  }

  return Result;
}

internal win32_game_code Win32LoadGameCode(char *GameDLLName) {
    win32_game_code Result = {0};

    char *GameTempDLLName = "game_temp.dll";
    //#ERROR#Result.LastDLLWriteTime = Win32GetLastFileWriteTime(GameDLLName);
    CopyFile(GameDLLName, GameTempDLLName, FALSE);
    Result.CodeDLL = LoadLibraryA("game_temp.dll");
    if (Result.CodeDLL) {
        Result.UpdateAndRender = (game_update_and_render *)
            GetProcAddress(Result.CodeDLL, "GameUpdateAndRender");
                //... other functions

        Result.IsValid = (Result.UpdateAndRender && 1);
    }

    if (!Result.IsValid) {
        Result.UpdateAndRender = GameUpdateAndRenderStub;
            //... other functions
    }

    return Result;
}

internal void Win32UnLoadGameCode(win32_game_code *GameCode) {
    if (GameCode->CodeDLL != INVALID_HANDLE_VALUE) {
        FreeLibrary(GameCode->CodeDLL);
        GameCode->CodeDLL = {0};
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRender = GameUpdateAndRenderStub;
        //... other functions
}

int CALLBACK WinMain (
  HINSTANCE instance,
  HINSTANCE prevInstance,
  LPSTR     cmdLine,
  int       cmdShow
) {
    const int ScreenWidth  = 1280;
    const int ScreenHeight = 720;

#if BUILD_INTERNAL
    LPVOID BaseAddRess = (LPVOID)terabytes((uint64)1);
#else
    LPVOID BaseAddRess = 0;
#endif

    game_mem GameMem = {0};
    GameMem.PermaStorageSize = megabytes(64);
    GameMem.TransStorageSize = gigabytes(1);

    GameMem.DEBUGPlatformFreeWholeFile = DEBUGPlatformFreeWholeFile;    
    GameMem.DEBUGPlatformReadWholeFile = DEBUGPlatformReadWholeFile;
    GameMem.DEBUGPlatformWriteWholeFile = DEBUGPlatformWriteWholeFile;

    uint64 totalSize = GameMem.PermaStorageSize + GameMem.TransStorageSize;
    GameMem.PermaStorageBytes = VirtualAlloc(BaseAddRess, (size_t)totalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    GameMem.TransStorageBytes = ((uint8*)GameMem.PermaStorageBytes + GameMem.PermaStorageSize);
    GameMem.IsInitialized = false;

    if (GameMem.PermaStorageBytes && GameMem.TransStorageBytes) {
        InitWindow(ScreenWidth, ScreenHeight, "window");
        SetTargetFPS(60);
            char *SourceDLLName = "game.dll";
            win32_game_code Game = Win32LoadGameCode(SourceDLLName);
            uint32 GameCodeLoadCount = 0;

            while (!WindowShouldClose()) {
                if (GameCodeLoadCount++ < 240) {
                    //#ERROR#FILETIME NewDLLWriteTime = Win32GetLastFileWriteTime(SourceDLLName);
                    //#ERROR#if (CompareFileTime(&Game.LastDLLWriteTime, &NewDLLWriteTime)) {
                        Win32UnLoadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceDLLName);
                    //#ERROR#}
                    GameCodeLoadCount = 0;
                }

                BeginDrawing();
                    ClearBackground(BLACK);
                    Game.UpdateAndRender(&GameMem);
                EndDrawing();
            }
        CloseWindow();
    }
    return 0;
}
