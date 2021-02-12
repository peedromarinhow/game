#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

#include "lingo.h"
#include "maths.h"
#include "platform.h"

void OutputSineWave(i16 *Samples, i32 SamplesPerSecond, i32 SampleCount,
                    i32 ToneFrequency, f32 tSine)
{
    i16 ToneVolume = 3000;
    i32 WavePeriod = SamplesPerSecond / ToneFrequency;

    i16* SampleOut = Samples;
    for (i32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex) {
        i16 SampleValue = 0;
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
        tSine += 2.0f * PI32 / (r32)WavePeriod;
        if (tSine > (2.0f * PI32)) {
            tSine -= 2.0f * PI32;
        }
    }
}

__declspec(dllexport) APP_UPDATE(AppUpdate) {
    // localpersist f32 tSine = 0.0f;
    // OutputSineWave(Platform->Samples, Platform->SamplesPerSecond, Platform->SampleCount, 440+Platform->dMouseWheel, tSine);
    glBegin(GL_LINES);

    glColor3f(1.0f, 1.0f, 1.0f);

    glVertex2f(Platform->MousePos.X/(r32)(Platform->WindowSize.X)      , Platform->MousePos.Y/(r32)(Platform->WindowSize.Y));
    glVertex2f(Platform->MousePos.X/(r32)(Platform->WindowSize.X + 100), Platform->MousePos.Y/(r32)(Platform->WindowSize.Y + 100));

    // glVertex2f(-1.0f, 0.0f);
    // glVertex2f( 1.0f, 0.0f);

    // glVertex2f(0.0f, -1.0f);
    // glVertex2f(0.0f,  1.0f);

    glEnd();
}
