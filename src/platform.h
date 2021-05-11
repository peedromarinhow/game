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

typedef struct _file_group {
    c8 **Filenames;
    u32  NoFiles;
} file_group;

typedef struct _image {
    void *Data;
    i32 w;
    i32 h;
    i32 Format;
} image;

typedef struct _texture {
    u32 Id;
    i32 w;
    i32 h;
    i32 Format;
} texture;

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

#define PLATFORM_GET_DIR_FILENAMES(Name) file_group Name(c8 *Dir)
typedef PLATFORM_GET_DIR_FILENAMES(platform_get_dir_filenames);

#define PLATFORM_REPORT_ERROR(Name) void Name(c8 *Title, c8 *ErrorMessage)
typedef PLATFORM_REPORT_ERROR(platform_report_error_callback);

#define PLATFORM_REPORT_ERROR_AND_DIE(Name) void Name(c8 *Title, c8 *ErrorMessage)
typedef PLATFORM_REPORT_ERROR_AND_DIE(platform_report_error_and_die_callback);

#define PLATFORM_GRAPHICS_CLEAR(Name) void Name(rv2 TargetDim, color Color)
typedef PLATFORM_GRAPHICS_CLEAR(platform_graphics_clear_callback);

#define PLATFORM_GRAPHICS_CLIP(Name) void Name(rect ClipRect)
typedef PLATFORM_GRAPHICS_CLIP(platform_graphics_clip_callback);

#define PLATFORM_GRAPHICS_RASTER_RECT(Name) void Name(rect Rect, color Color)
typedef PLATFORM_GRAPHICS_RASTER_RECT(platform_graphics_raster_rect_callback);

#define PLATFORM_GRAPHICS_RASTER_TEXTURE_RECT(Name) void Name(rv2 Pos, rect Rect, texture Texture, color Tint)
typedef PLATFORM_GRAPHICS_RASTER_TEXTURE_RECT(platform_graphics_raster_texture_rect_callback);

#define PLATFORM_GRAPHICS_ENABLE(Name) void Name(u32 Opt)
typedef PLATFORM_GRAPHICS_ENABLE(platform_graphics_enable_callback);

#define PLATFORM_GRAPHICS_DISABLE(Name) void Name(u32 Opt)
typedef PLATFORM_GRAPHICS_DISABLE(platform_graphics_disable_callback);

#define PLATFORM_GRAPHICS_GEN_AND_BIND_AND_LOAD_TEXTURE(Name) void Name(image *Image, texture *Texture)
typedef PLATFORM_GRAPHICS_GEN_AND_BIND_AND_LOAD_TEXTURE(platform_graphics_gen_and_bind_and_load_texture_callback);

#define PLATFORM_GRAPHICS_BLEND_FUNC(Name) void Name(void)
typedef PLATFORM_GRAPHICS_BLEND_FUNC(platform_graphics_blend_func_callback);

typedef struct _platform_api {
    platform_allocate_memory_callback *AllocateMemory;
    platform_free_memory_callback *FreeMemory;
    platform_load_file_callback *LoadFile;
    platform_free_file_callback *FreeFile;
    platform_load_file_to_arena_callback *LoadFileToArena;
    platform_free_file_from_arena_callback *FreeFileFromArena;
    platform_write_file_callback *WriteFile;
    platform_get_dir_filenames *GetDirFilenames;
    platform_report_error_callback *ReportError;
    platform_report_error_and_die_callback *ReportErrorAndDie;

    platform_graphics_clear_callback *Clear;
    platform_graphics_clip_callback *Clip;
    platform_graphics_raster_rect_callback *RasterRect;
    platform_graphics_raster_texture_rect_callback *RasterTextureRect;
    platform_graphics_enable_callback *Enable;
    platform_graphics_disable_callback *Disable;
    platform_graphics_gen_and_bind_and_load_texture_callback *GenAndBindAndLoadTexture;
    platform_graphics_blend_func_callback *BlendFunc;
} platform_api;

typedef struct _button_state {
    i32 HalfTransitionCount;
    b32 EndedDown;
} button_state;

enum plat_key {
    plat_KEYBEV_CHAR = 0,
    plat_KEYB_UP,
    plat_KEYB_DOWN,
    plat_KEYB_LEFT,
    plat_KEYB_RIGHT,
    plat_KEYB_HOME,
    plat_KEYB_END,
    plat_KEYB_PG_UP,
    plat_KEYB_PG_DOWN,
    plat_KEYB_BACK,
    plat_KEYB_DELETE,
    plat_KEYB_TAB,
    plat_KEYB_RETURN,
    plat_KEYB_CTRL,
    plat_KEYB_SHIFT,
    plat_KEYB_ALT,
    plat_KEYMEV_MOVED,
    plat_KEYM_RIGHT,
    plat_KEYM_LEFT,
    plat_KEYM_MIDDLE,
    plat_NO_KEYS = plat_KEYM_MIDDLE
};

//note: this is how the platform and the app communicate with each other.
typedef struct _platform {
    // metadata
    c8 *ExecutablePath;
    c8 *WorkingDirectoryPath;
    // options
    b32 Fullscreen;
    b32 Running;
// b32 WindowResized;
    iv2 WindowDim;
    r32 dtForFrame;
    // mouse input
    rv2  MousePos;
    i16 dMouseWheel;
    // mouse and keyboard buttons
    button_state Buttons[plat_NO_KEYS];
    u32 Char; //todo: unicode
    // memory
    app_memory Memory;
    // functions
    platform_api Api;
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

#endif//PLATFORM_H
