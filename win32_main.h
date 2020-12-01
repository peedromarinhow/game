#ifndef MAIN_H
#define MAIN_H



typedef struct _win32_game_code {
    HMODULE CodeDLL;
    FILETIME LastDLLWriteTime;
    game_update_and_render *UpdateAndRender;
        // ... other functions
    bool IsValid;
} win32_game_code;

typedef struct _win32_state {

} win32_state;



#endif // MAIN_H
