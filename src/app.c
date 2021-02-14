#include <windows.h>
#include <gl/gl.h>

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

//todo: APP_ONLOAD function that runs when the app is loaded
//note:
//  this will require app memory, so that the vulkan stuff
//  can be stored in it and shared between the functions

__declspec(dllexport) APP_UPDATE(AppUpdate) {
    glViewport(0, 0, Platform->WindowSize.Width, Platform->WindowSize.Height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    r32 a = 2.0f/Platform->WindowSize.x;
    r32 b = 2.0f/Platform->WindowSize.y;
    r32 Proj[] = {
         a,  0,  0,  0,
         0, -b,  0,  0,
         0,  0,  1,  0,
        -1,  1,  0,  1
    };
    glLoadMatrixf(Proj);

    glBegin(GL_LINES); {

        glColor3f(1.0f, 1.0f, 1.0f);

        rv2 LowerLeftCorner = {
            (Platform->MousePos.x),
            (Platform->MousePos.y)
        };

        rv2 UpperRightCorner = {
            (Platform->MousePos.x + 100.0f),
            (Platform->MousePos.y + 100.0f)
        };

        glVertex2f(LowerLeftCorner.x, LowerLeftCorner.y);
        glVertex2f(UpperRightCorner.x, UpperRightCorner.y);

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

            if ((100 <= Platform->MousePos.x && Platform->MousePos.x <= 200) &&
                (100 <= Platform->MousePos.y && Platform->MousePos.y <= 200) &&
                Platform->Mouse.Left.EndedDown)
            {                
                glColor3f(1.0f, 0.0f, 1.0f);
            }

            glVertex2f(P1.x, P1.y);
            glVertex2f(P2.x, P2.y);
            glVertex2f(P3.x, P3.y);

        } glEnd();
    }

    glBegin(GL_LINES); {

        glColor3f(1.0f, 1.0f, 1.0f);

        rv2 P1 = {200, 200};
        rv2 P2 = {300, 300};

        glVertex2f(P1.x, P1.y);
        if (Platform->CharacterInput == 'a')
            glVertex2f(P2.x, P2.y);

    } glEnd();

    r32 ComplexProj[] = {
        1,  0,  0,  0,
        0,  1,  0,  0,
        0,  0,  1,  0,
        0,  0,  0,  1
    };
    glLoadMatrixf(ComplexProj);

    glBegin(GL_LINES); {
        glVertex2f(0,  1000);
        glVertex2f(0, -1000);

        glVertex2f( 1000, 0);
        glVertex2f(-1000, 0);

        c32 Origin = (c32){0, 0};
        c32 z = (c32){100, 100};
        glVertex2f(Origin.a, Origin.b);
        glVertex2f(z.a, z.b);

        c32 w = ExpITheta(Platform->MousePos.x/100);
        glVertex2f(Origin.a, Origin.b);
        glVertex2f(w.a, w.b);
    } glEnd();
}
