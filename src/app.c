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
    glBegin(GL_LINES); {

        glColor3f(1.0f, 1.0f, 1.0f);

        rv2 LowerLeftCorner = {
            (Platform->MousePos.X),
            (Platform->MousePos.Y)
        };

        rv2 UpperRightCorner = {
            (Platform->MousePos.X + 100.0f),
            (Platform->MousePos.Y + 100.0f)
        };

        glVertex2f(LowerLeftCorner.X, LowerLeftCorner.Y);
        glVertex2f(UpperRightCorner.X, UpperRightCorner.Y);

    } glEnd();

    if (Platform->Mouse.Left.EndedDown  ||
        Platform->Mouse.Right.EndedDown ||
        Platform->Mouse.Middle.EndedDown)
    {
        glBegin(GL_TRIANGLES); {

            glColor3f(1.0f, 1.0f, 1.0f);

            rv2 P1 = {100, 100};
            rv2 P2 = {200, 100};
            rv2 P3 = {100, 200};

            glVertex2f(P1.X, P1.Y);
            glVertex2f(P2.X, P2.Y);
            glVertex2f(P3.X, P3.Y);

        } glEnd();
    }

    glBegin(GL_LINES); {

        glColor3f(1.0f, 1.0f, 1.0f);

        localpersist rv2 P1 = {200, 200};
        localpersist rv2 P2 = {300, 300};

        P2.X += (r32)Platform->dMouseWheel;
        P2.Y += (r32)Platform->dMouseWheel;

        glVertex2f(P1.X, P1.Y);
        glVertex2f(P2.X, P2.Y);

    } glEnd();
}
