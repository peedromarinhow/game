#ifndef WIN32_MAIN_H
#define WIN32_MAIN_H

typedef struct _win32_offscreen_buffer
{
    BITMAPINFO Info;
    void  *Memory;
    int32 Width;
    int32 Height;
    int32 Pitch;
    int32 BytesPerPixel;
}
win32_offscreen_buffer;

typedef struct _win32_window_dimensions
{
    int32 Width;
    int32 Height;
}
win32_window_dimensions;

typedef struct _win32_sound_output
{
    int32  SamplesPerSecond;
    int32  BytesPerSample;
    uint32 RunningSampleIndex;
    DWORD  SecondaryBufferSize;
    DWORD  SafetyBytes;
    real32 SineT;
    //todo
    //  maths will get easier with a "BytesPerSecond" field
}
win32_sound_output;

typedef struct _DEBUG_win32_time_marker
{
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;  
    DWORD OutputByteCount;

    DWORD ExpectedFlipPlayCursor;
    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
}
DEBUG_win32_time_marker;

typedef struct _win32_game_code
{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    // important
    //  either of these functions can be null!
    //  check before calling!
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;

  bool32 IsValid;
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
    uint64 TotalSize;
    void *GameMemoryBlock;
    win32_replay_buffer ReplayBuffers[2];

    HANDLE RecordingHandle;
    int32  InputRecordingIndex;

    HANDLE PlaybackHandle;
    int32  InputPlaybackIndex;

    char EXEFileName[WIN32_STATE_FILENAME_COUNT];
    char *OnePastLastEXEFileNameSlash;
}
win32_state;

#endif//WIN32_MAIN_H