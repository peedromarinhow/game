#ifndef GAME_H
#define GAME_H



typedef struct _game_mem {
    bool   IsInitialized;

    uint64 PermaStorageSize;
    void*  PermaStorageBytes; // REQUIRED to be cleared to zero on every platform

    uint64 TransStorageSize;
    void*  TransStorageBytes; // ALSO REQUIRED to be cleared to zero on every platform

    debug_platform_free_whole_file *DEBUGPlatformFreeWholeFile;
    debug_platform_read_whole_file *DEBUGPlatformReadWholeFile;
    debug_platform_write_whole_file *DEBUGPlatformWriteWholeFile;
} game_mem;

typedef struct _game_state {
    Rectangle Rect;
    int ScreenHeight;
    int ScreenWidth;
} game_state;

#define GAME_UPDATE_AND_RENDER(name) void name(game_mem *GameMem)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub) {
}



#endif // GAME_H
