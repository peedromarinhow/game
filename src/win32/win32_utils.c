#include "lingo.h"
#include "platform.h"
#include "memory.h"

//note: all this is basically _stolen_ from ryan's platform layer

//todo: check for fails, etc
PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory) {
    return VirtualAlloc(0, (size_t)Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

PLATFORM_FREE_MEMORY(Win32FreeMemory) {
    VirtualFree(Data, sizeof(Data), MEM_RELEASE);
}

//todo: varargs for formats on these two functions
PLATFORM_REPORT_ERROR(Win32ReportError) {
    MessageBoxA(0, ErrorMessage, Title, MB_OK);
}

PLATFORM_REPORT_ERROR_AND_DIE(Win32ReportErrorAndDie) {
    MessageBoxA(0, ErrorMessage, Title, MB_OK);
    _Exit(1);
}

PLATFORM_FREE_FILE(Win32FreeFile) {
    VirtualFree(File.Data, File.Size, MEM_RELEASE);
}

//note: basically copied from ryan's
PLATFORM_LOAD_FILE(Win32LoadFile) {
    file Result;
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ | GENERIC_WRITE,
                                    0, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE) {
        DWORD ReadBytes = GetFileSize(FileHandle, 0);
        if (ReadBytes != INVALID_FILE_SIZE) {
            void *ReadData = VirtualAlloc(NULL, (size_t)ReadBytes + 1,
                                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            DWORD BytesRead = 0;

            ReadFile(FileHandle, ReadData, ReadBytes, &BytesRead, NULL);
            //todo: figure out why the file is read but BytesRead keeps set to zero

            ((u8 *)ReadData)[ReadBytes] = 0;

            Result.Data = ReadData;
            Result.Size = (u64)BytesRead;
        }
        CloseHandle(FileHandle);
    }
    return Result;
}

PLATFORM_FREE_FILE_FROM_ARENA(Win32FreeFileFromArena) {
    PopFromArena(Arena, File.Size);
}

PLATFORM_LOAD_FILE_TO_ARENA(Win32LoadFileToArena) {
    file Result;
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ | GENERIC_WRITE,
                                    0, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE) {
        DWORD ReadBytes = GetFileSize(FileHandle, 0);
        if (ReadBytes != INVALID_FILE_SIZE) {
            void *ReadData = PushToArena(Arena, ReadBytes + 1);
            DWORD BytesRead = 0;

            ReadFile(FileHandle, ReadData, ReadBytes, &BytesRead, NULL);
            //todo: figure out why the file is read but BytesRead keeps set to zero

            ((u8 *)ReadData)[ReadBytes] = 0;

            Result.Data = ReadData;
            Result.Size = (u64)BytesRead;
        }
        CloseHandle(FileHandle);
    }
    return Result;
}

PLATFORM_WRITE_FILE(Win32WriteFile) {
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ | GENERIC_WRITE,
                                    0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        void *DataToWrite     = Data;
        DWORD DataToWriteSize = Size;
        DWORD BytesWritten    = 0;

        WriteFile(FileHandle, DataToWrite, DataToWriteSize, &BytesWritten, NULL);
        
        CloseHandle(FileHandle);
    }
    else {
        Win32ReportError("ERROR", "Could not save to file");
        //Win32ReportError("ERROR", "Could not save to file\"%s\"\n", Filename);
    }
}

internal void Win32ToggleFullScreen(HWND Window) {
    localpersist WINDOWPLACEMENT WindowPosition = {sizeof(WindowPosition)};
    //note:
    //  copied from https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    //  by Raymond Chen
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if (GetWindowPlacement(Window, &WindowPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &WindowPosition);
        SetWindowPos(Window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}
