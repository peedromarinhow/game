#include "windows_and_its_defines.h"
#include <raylib.h>

#include "game.h"
#include "win32_main.h"



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

internal win32_game_code Win32LoadGameCode(char *SourceDLLName, char *TempDLLName) {
    win32_game_code Result = {0};

    Result.LastDLLWriteTime = Win32GetLastFileWriteTime(SourceDLLName);

    while(1) {
        if(CopyFile(SourceDLLName, TempDLLName, FALSE)) break;
        if(GetLastError() == ERROR_FILE_NOT_FOUND) break;
    }
    
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

internal void ProcessKeyPress(/*game_state *State*/game_mem *GameMem) {
    assert(sizeof(game_state) <= GameMem->PermaStorageSize);
    game_state *State = (game_state *)GameMem->PermaStorageBytes;
    float Increment = 1500.0f;
    if (IsKeyDown(KEY_W) && (State->Rect.y > 0))
        State->Rect.y -= GetFrameTime()*Increment;
    if (IsKeyDown(KEY_S) && (State->Rect.y < State->ScreenHeight))
        State->Rect.y += GetFrameTime()*Increment;
    if (IsKeyDown(KEY_A) && (State->Rect.x > 0))
        State->Rect.x -= GetFrameTime()*Increment;
    if (IsKeyDown(KEY_D) && (State->Rect.x < State->ScreenWidth))
        State->Rect.x += GetFrameTime()*Increment;
}

// absolute trash this is
void StrCat(
  size_t SourceACount, char *SourceA,
  size_t SourceBCount, char *SourceB,
  size_t DestCount   , char *Dest
) {
    for (size_t Index = 0; Index < SourceACount; Index++) {
        *Dest++ = *SourceA++;
    }
    for (size_t Index = 0; Index < SourceBCount; Index++) {
        *Dest++ = *SourceB++;
    }

    *Dest++ = '\0';
}

int CALLBACK WinMain (
  HINSTANCE instance,
  HINSTANCE prevInstance,
  LPSTR     cmdLine,
  int       cmdShow
) {
    char EXEFileName[MAX_PATH];
    DWORD SizeofFileName = GetModuleFileNameA(0, EXEFileName, sizeof(EXEFileName));
    char *OnePastLastSlash = EXEFileName;
    for (char *Scan = EXEFileName; *Scan; ++Scan) {
        if (*Scan == '\\') {
            OnePastLastSlash = Scan + 1;
        }
    }
    
    char SourceGameCodeDLLFileName[] = "game.dll";
    char SourceGameCodeDLLFullPath[MAX_PATH];
    StrCat(
        OnePastLastSlash - EXEFileName, EXEFileName,
        sizeof(SourceGameCodeDLLFileName) - 1, SourceGameCodeDLLFileName,
        sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath
    );

    char TempGameCodeDLLFileName[] = "game_temp.dll";
    char TempGameCodeDLLFullPath[MAX_PATH];
    StrCat(
        OnePastLastSlash - EXEFileName, EXEFileName,
        sizeof(TempGameCodeDLLFileName) - 1, TempGameCodeDLLFileName,
        sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath
    );

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
        int InputRecordingIndex = 0;
        int InputPlayingIndex = 0;

            win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);

            while (!WindowShouldClose()) {
                FILETIME NewDLLWriteTime = Win32GetLastFileWriteTime(SourceGameCodeDLLFullPath);
                if (CompareFileTime(&Game.LastDLLWriteTime, &NewDLLWriteTime)) {
                    Win32UnLoadGameCode(&Game);
                    Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
                }

                BeginDrawing();
                    ClearBackground(BLACK);
                    Game.UpdateAndRender(&GameMem);
                    ProcessKeyPress(&GameMem);
                EndDrawing();
            }
        CloseWindow();
    }
    return 0;
}
