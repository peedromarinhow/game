#ifndef MAIN_H
#define MAIN_H



typedef struct _win32_game_code {
    HMODULE CodeDLL;
    FILETIME LastDLLWriteTime;
    game_update_and_render *UpdateAndRender;
        // ... other functions
    bool IsValid;
} win32_game_code;



#endif // MAIN_H
