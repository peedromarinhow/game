#include "lingo.h"

//todo: put this on some string file
void CatStrings(size_t SourceACount, char *SourceA,
                size_t SourceBCount, char *SourceB,
                size_t DestCount, char *Dest)
{
    for (size_t Index = 0; Index < SourceACount; Index++) {
        *Dest++ = *SourceA++;
    }
    for (size_t Index = 0; Index < SourceBCount; Index++) {
        *Dest++ = *SourceB++;
    }

    *Dest++ = '\0';
}

internal void Win32BuildEXEPathFilename(char *Dest, i32 DestCount, char *Filename,
                                        char *OnePastLastSlash, char *ExecutablePath)
{
    CatStrings(OnePastLastSlash - ExecutablePath,
               ExecutablePath, StringLenght(Filename), Filename,
               DestCount, Dest);
}