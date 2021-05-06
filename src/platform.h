#ifndef PLATFORM_H
#define PLATFORM_H

#include "lingo.h"
#include "maths.h"

//note: these functions are to be implemented in each platform and passed by the app via "platform"

typedef struct _app_memory {
    void *Contents;
    u32   Size;
} app_memory;

typedef struct _file {
    void *Data;
    u64   Size;
} file;

typedef struct _memory_arena {
    u64   MaxSize;
    u64   Used;
    void *Base;
} memory_arena;

internal memory_arena InitializeArena(u64 MaxSize, void *Base) {
    memory_arena Arena = {0};
    Arena.MaxSize = MaxSize;
    Arena.Base    = Base;
    Arena.Used    = 0;
    return Arena;
}

internal void *PushToArena(memory_arena *Arena, u64 Size) {
    void *Memory = 0;
    if ((Arena->Used + Size) < Arena->MaxSize) {
        Memory = (u8 *)Arena->Base + Arena->Used;
        Arena->Used += Size;
    }
    return Memory;
}

internal void PopFromArena(memory_arena *Arena, u64 Size) {
    if (Size < Arena->Used) {
        Size = Arena->Used;
    }
    Arena->Used -= Size;
}

internal void ClearArena(memory_arena *Arena) {
    PopFromArena(Arena, Arena->Used);
}

#define PLATFORM_ALLOCATE_MEMORY(Name) void *Name(u32 Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory_callback);

#define PLATFORM_FREE_MEMORY(Name) void Name(void *Data)
typedef PLATFORM_FREE_MEMORY(platform_free_memory_callback);

#define PLATFORM_LOAD_FILE(Name) file Name(c8 *Filename)
typedef PLATFORM_LOAD_FILE(platform_load_file_callback);

#define PLATFORM_FREE_FILE(Name) void Name(file File)
typedef PLATFORM_FREE_FILE(platform_free_file_callback);

#define PLATFORM_LOAD_FILE_TO_ARENA(Name) file Name(memory_arena *Arena, c8 *Filename)
typedef PLATFORM_LOAD_FILE_TO_ARENA(platform_load_file_to_arena_callback);

#define PLATFORM_FREE_FILE_FROM_ARENA(Name) void Name(memory_arena *Arena, file File)
typedef PLATFORM_FREE_FILE_FROM_ARENA(platform_free_file_from_arena_callback);

#define PLATFORM_WRITE_FILE(Name) void Name(void *Data, u32 Size, c8 *Filename, b32 Append)
typedef PLATFORM_WRITE_FILE(platform_write_file_callback);

typedef struct _file_group {
    c8 **Filenames;
    u32  NoFiles;
} file_group;

#define PLATFORM_GET_DIR_FILENAMES(Name) file_group Name(c8 *Dir)
typedef PLATFORM_GET_DIR_FILENAMES(platform_get_dir_filenames);

#define PLATFORM_REPORT_ERROR(Name) void Name(c8 *Title, c8 *ErrorMessage)
typedef PLATFORM_REPORT_ERROR(platform_report_error_callback);

#define PLATFORM_REPORT_ERROR_AND_DIE(Name) void Name(c8 *Title, c8 *ErrorMessage)
typedef PLATFORM_REPORT_ERROR_AND_DIE(platform_report_error_and_die_callback);

typedef struct _button_state {
    i32 HalfTransitionCount;
    b32 EndedDown;
} button_state;

typedef struct _event_state {
    i32 HalfTransitionCount;
    b32 EndedHappening;
} event_state;

//note: this is how the platform and the app communicate with each other.
#define KEYBOARD_MAX_KEYS 17
#define MOUSE_MAX_BUTTONS  3
typedef struct _platform {
    /* metadata */
    c8 *ExecutablePath;
    c8 *WorkingDirectoryPath;

    /* options */
    b32 Fullscreen;
    b32 Running;
    b32 WindowResized;
    iv2 WindowDim;
    r32 dtForFrame;

    /* mouse input */
    b32 mMoved;
    rv2 mPos;
    b32 mLeft, mRight, mMiddle;
    i16 dmWheel;

    /* keyboard input */
    union {
        b32 kKeys[KEYBOARD_MAX_KEYS];
        struct {
            b32 kUp, kDown, kLeft, kRight;
            b32 kHome, kEnd;
            b32 kPgUp, kPgDown;
            b32 kBack, kDelete, kTab, kReturn;
            b32 kCtrl, kShift,  kAlt;
            b32 kChar;
            c8  Char;
        };
    };

    /* gamepad */
    //todo
    
    /* sound */
    // i16 *Samples;
    // i32  SamplesPerSecond;
    // i32  SampleCount;

    /* memory */
    app_memory Memory;

    /* functions */
    platform_allocate_memory_callback      *AllocateMemoryCallback;
    platform_free_memory_callback          *FreeMemoryCallback;
    platform_load_file_callback            *LoadFileCallback;
    platform_free_file_callback            *FreeFileCallback;
    platform_load_file_to_arena_callback   *LoadFileToArenaCallback;
    platform_free_file_from_arena_callback *FreeFileFromArenaCallback;
    platform_write_file_callback           *WriteFileCallback;
    platform_get_dir_filenames             *GetDirFilenames;
    platform_report_error_callback         *ReportErrorCallback;
    platform_report_error_and_die_callback *ReportErrorAndDieCallback;
} platform;

#define APP_INIT(Name) void Name(platform *p)
typedef APP_INIT(app_init_callback);
        APP_INIT(AppInitStub) {};

#define APP_RELOAD(Name) void Name(platform *p)
typedef APP_RELOAD(app_reload_callback);
        APP_RELOAD(AppReloadStub) {};

#define APP_UPDATE(Name) void Name(platform *p)
typedef APP_UPDATE(app_update_callback);
        APP_UPDATE(AppUpdateStub) {};

#define APP_DEINIT(Name) void Name(platform *p)
typedef APP_DEINIT(app_deinit_callback);
        APP_DEINIT(AppDeinitStub) {};

typedef struct _platform_api {
    platform_allocate_memory_callback      *AllocateMemory;
    platform_free_memory_callback          *FreeMemory;
    platform_load_file_callback            *LoadFile;
    platform_free_file_callback            *FreeFile;
    platform_load_file_to_arena_callback   *LoadFileToArena;
    platform_free_file_from_arena_callback *FreeFileFromArena;
    platform_write_file_callback           *WriteFile;
    platform_get_dir_filenames             *GetDirFilenames;
    platform_report_error_callback         *ReportError;
    platform_report_error_and_die_callback *ReportErrorAndDie;
} platform_api;

inline platform_api SetPlatformApi(platform *p) {
    platform_api Result = {0};

    Result.AllocateMemory    = p->AllocateMemoryCallback;
    Result.FreeMemory        = p->FreeMemoryCallback;
    Result.LoadFile          = p->LoadFileCallback;
    Result.FreeFile          = p->FreeFileCallback;
    Result.LoadFileToArena   = p->LoadFileToArenaCallback;
    Result.FreeFileFromArena = p->FreeFileFromArenaCallback;
    Result.WriteFile         = p->WriteFileCallback;
    Result.GetDirFilenames   = p->GetDirFilenames;
    Result.ReportError       = p->ReportErrorCallback;
    Result.ReportErrorAndDie = p->ReportErrorAndDieCallback;

    return Result;
}

#endif//PLATFORM_H
