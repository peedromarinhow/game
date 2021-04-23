#ifndef APP_H
#define APP_H

#include "lingo.h"
#include "platform.h"

typedef struct _platform_api {
    platform_allocate_memory_callback            *AllocateMemory;
    platform_free_memory_callback                *FreeMemory;
    platform_load_file_callback                  *LoadFile;
    platform_free_file_callback                  *FreeFile;
    platform_load_file_to_arena_callback      *LoadFileToArena;
    platform_free_file_from_arena_callback    *FreeFileFromArena;
    platform_write_file_callback                 *WriteFile;
    // platform_get_all_filenames_from_dir_callback *GetAllFilenamesFromDir;
    platform_report_error_callback               *ReportError;
    platform_report_error_and_die_callback       *ReportErrorAndDie;
} platform_api;

#endif//APP_H