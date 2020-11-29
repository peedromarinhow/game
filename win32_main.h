#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
/*#define NONLS             // All NLS defines and routines*/
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines



#ifndef MAIN_H
#define MAIN_H

#include "stdint.h"



#if BUILD_SLOW
    #define assert(e) if (!(e)) { *(int*)0 = 0; }
#else
    #define assert(e)
#endif



#define internal   static
#define global     static
#define persistent static



#define kilobytes(n) ((n)*1024LL)
#define megabytes(n) (kilobytes(n)*1024LL)
#define gigabytes(n) (megabytes(n)*1024LL)
#define terabytes(n) (gigabytes(n)*1024LL)

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

inline uint32 SafeTruncateUint64(uint64 val) {
    uint32 result = 0;
    if (val <= 0xFFFFFFFF) result = (uint32)val;
    return result;
}



#if BUILD_INTERNAL

typedef struct _debug_read_file_result {
    uint64 ContentsSize;
    void *Contents;
} debug_read_file_result;

#define DEBUG_PLATFORM_FREE_WHOLE_FILE(name) void name(void* Mem)
typedef DEBUG_PLATFORM_FREE_WHOLE_FILE(debug_platform_free_whole_file);

#define DEBUG_PLATFORM_READ_WHOLE_FILE(name) debug_read_file_result name(char* Filename)
typedef DEBUG_PLATFORM_READ_WHOLE_FILE(debug_platform_read_whole_file);

#define DEBUG_PLATFORM_WRITE_WHOLE_FILE(name) bool name(char* Filename, void* Mem, uint32 MemSize)
typedef DEBUG_PLATFORM_WRITE_WHOLE_FILE(debug_platform_write_whole_file);

#else
#endif



#endif // MAIN_H
