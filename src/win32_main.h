#ifndef WIN32_MAIN_H
#define WIN32_MAIN_H

typedef struct _win32_offscreen_buffer
{
    BITMAPINFO Info;
    void  *Memory;
    i32 Width;
    i32 Height;
    i32 Pitch;
    i32 BytesPerPixel;
}
win32_offscreen_buffer;

typedef struct _win32_window_dimensions
{
    i32 Width;
    i32 Height;
}
win32_window_dimensions;

typedef struct _win32_sound_output
{
    i32  SamplesPerSecond;
    i32  BytesPerSample;
    u32 RunningSampleIndex;
    DWORD  SecondaryBufferSize;
    DWORD  SafetyBytes;
    r32 SineT;
    //todo
    //  maths will get easier with a "BytesPerSecond" field
}
win32_sound_output;

typedef struct _debug_win32_time_marker
{
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;  
    DWORD OutputByteCount;

    DWORD ExpectedFlipPlayCursor;
    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
}
debug_win32_time_marker;

typedef struct _win32_game_code
{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    // important
    //  either of these functions can be null!
    //  check before calling!
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;

    b32 IsValid;
}
win32_game_code;

#define WIN32_STATE_FILENAME_COUNT MAX_PATH

typedef struct _win32_replay_buffer
{
    HANDLE FileHandle;
    HANDLE MemoryMap;
    char Filename[WIN32_STATE_FILENAME_COUNT];
    void *MemoryBlock;
}
win32_replay_buffer;

typedef struct _win32_state
{
    u64 TotalSize;
    void *GameMemoryBlock;
    win32_replay_buffer ReplayBuffers[2];

    HANDLE RecordingHandle;
    i32  InputRecordingIndex;

    HANDLE PlaybackHandle;
    i32  InputPlaybackIndex;

    char EXEFileName[WIN32_STATE_FILENAME_COUNT];
    char *OnePastLastEXEFileNameSlash;
}
win32_state;

#endif//WIN32_MAIN_H