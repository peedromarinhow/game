#ifndef APP_H
#define APP_H

#include "lingo.h"
#include "platform.h"

global platform_allocate_memory_callback      *AllocateMemory;
global platform_free_memory_callback          *FreeMemory;
global platform_load_file_callback            *LoadFile;
global platform_free_file_callback            *FreeFile;
global platform_load_file_to_arena_callback   *LoadFileToArena;
global platform_free_file_from_arena_callback *FreeFileFromArena;
global platform_write_file_callback           *WriteFile_; //damn you, "windows.h"
global platform_report_error_callback         *ReportError;
global platform_report_error_and_die_callback *ReportErrorAndDie;

typedef struct _app_state {
    texture Temp;
} app_state;

#endif//APP_H