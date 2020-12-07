#ifndef GAME_H
#define GAME_H

#include <stdint.h>



typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
// typedef int32 bool32;
typedef float real32;
typedef double real64;



#define internal   static
#define global     static
#define persistent static



#define kilobytes(n) ((n)*1024ull)
#define megabytes(n) (kilobytes(n)*1024ull)
#define gigabytes(n) (megabytes(n)*1024ull)
#define terabytes(n) (gigabytes(n)*1024ull)



#if BUILD_SLOW
    #define assert(e) if (!(e)) { *(int*)0 = 0; }
#else
    #define assert(e)
#endif



inline uint32 SafeTruncateUint64(uint64 val) {
    uint32 result = 0;
    if (val <= 0xFFFFFFFF) result = (uint32)val;
    return result;
}



#if BUILD_INTERNAL
    typedef struct _debug_read_file_result {
        uint64 ContentsSize;
        void *Contents;
    } debug_read_file_result;

    #define DEBUG_PLATFORM_FREE_WHOLE_FILE(name) void name(void* Mem)
    typedef DEBUG_PLATFORM_FREE_WHOLE_FILE(debug_platform_free_whole_file);

    #define DEBUG_PLATFORM_READ_WHOLE_FILE(name) debug_read_file_result name(char* Filename)
    typedef DEBUG_PLATFORM_READ_WHOLE_FILE(debug_platform_read_whole_file);

    #define DEBUG_PLATFORM_WRITE_WHOLE_FILE(name) bool name(char* Filename, void* Mem, uint32 MemSize)
    typedef DEBUG_PLATFORM_WRITE_WHOLE_FILE(debug_platform_write_whole_file);
#else
#endif



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



#define GAME_UPDATE_AND_RENDER(name) void name(game_input *GameInput, game_mem *GameMem)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub) {
}



typedef struct _game_button_state {
    int32 HalfTransitionNo;
    bool  EndedDown;
} game_button_state;

typedef struct _game_controller_input {
    bool IsAnalog;

    real32 StartX;
    real32 StartY;

    real32 MinX;
    real32 MinY;

    real32 MaxX;
    real32 MaxY;

    real32 EndX;
    real32 EndY;

    union {
        game_button_state Buttons[6];
        struct {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
} game_controller_input;

typedef struct _game_input {
    game_controller_input Controllers[4];
} game_input;




#endif // GAME_H
