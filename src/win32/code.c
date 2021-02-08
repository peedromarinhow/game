#include "lingo.h"

typedef struct _win32_app_code {
    HMODULE              DLL;
    FILETIME             LastDLLWriteTime;
    app_update_callback *Update;
} win32_app_code;

inline FILETIME GetFileLastWriteTime(char *Filename) {
    FILETIME Result = {0};
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
        Result = Data.ftLastWriteTime;
    return Result;
}

internal b32 Win32LoadAppCode(win32_app_code *Code,
                              char *DLLPath, char *TempDLLPath)
{
    b32 Success = 1;
    
    while (TRUE) {
        if (CopyFileA(DLLPath, TempDLLPath, FALSE))
            break;
    }

    Code->DLL = LoadLibraryA(TempDLLPath);
    Code->LastDLLWriteTime = GetFileLastWriteTime(DLLPath);
    if (!Code->DLL) {
        Success = 0;
        return Success;
    }

    Code->Update = (app_update_callback *)GetProcAddress(Code->DLL, "Update");
    if (!Code->Update) {
        Code->Update = AppUpdateStub;
        Success = 0;
        return Success;
    }

    return Success;
}

internal void Win32UnloadAppCode(win32_app_code *Code)
{
    if(Code->DLL)
        FreeLibrary(Code->DLL);
    Code->DLL = 0;
    Code->Update = AppUpdateStub;
}

internal void Win32UpdateAppCode(win32_app_code *Code, char *DLLPath, char *TempDLLPath) {
    FILETIME LastDLLWriteTime = GetFileLastWriteTime(DLLPath);
    if(CompareFileTime(&LastDLLWriteTime, &Code->LastDLLWriteTime)) {
        Win32UnloadAppCode(Code);
        Win32LoadAppCode(Code, DLLPath, TempDLLPath);
    }
}
