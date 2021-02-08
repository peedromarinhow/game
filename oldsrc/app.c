#include <gl/gl.h>
#include "app.h"

internal i32 RoundR32ToI32(r32 Real32) {
    return (i32)(Real32 + 0.5f);
}

internal u32 RoundR32ToUI32(r32 Real32) {
    return (u32)(Real32 + 0.5f);
}

internal i32 TruncateR32ToI32(r32 Real32) {
    return (i32)(Real32);
}

internal u32 TruncateR32ToUI32(r32 Real32) {
    return (u32)(Real32);
}

void DrawRectangle(app_video_buffer *Buffer,
                   r32 RealMinX, r32 RealMinY,
                   r32 RealMaxX, r32 RealMaxY,
                   r32 R, r32 G, r32 B) {
    i32 MinX = RoundR32ToI32(RealMinX);
    i32 MinY = RoundR32ToI32(RealMinY);
    i32 MaxX = RoundR32ToI32(RealMaxX);
    i32 MaxY = RoundR32ToI32(RealMaxY);

    u32 Color = (RoundR32ToUI32(R * 255.0f) << 16) |
                   (RoundR32ToUI32(G * 255.0f) << 8)  |
                   (RoundR32ToUI32(B * 255.0f) << 0);

    if (MinX < 0)
        MinX = 0;

    if (MinY < 0)
        MinY = 0;

    if (MaxX > Buffer->Width)
        MaxX = Buffer->Width;

    if (MaxY > Buffer->Height)
        MaxY = Buffer->Height;

    u8 *Row = (u8 *)Buffer->Memory + (MinX * Buffer->BytesPerPixel) + (MinY * Buffer->Pitch);
    for (i32 Y = MinY; Y < MaxY; Y++) {
        u32 *Pixel = (u32 *)Row;
        for (i32 X = MinX; X < MaxX; X++) {
            *Pixel++ = Color;
        }

        Row += Buffer->Pitch;
    }
}

inline void OpenGLRect(r32 P, u32 Color) {
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // lower tri
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-P,-P);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(P, -P);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(P, P);

    // higher tri
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-P, -P);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(P,P);

    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-P, P);
}

extern "C" APP_UPDATE(AppUpdateAndRender) {
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
        ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(app_state) <= Memory->PermanentStorageSize);
    
    glViewport(0, 0, VideoBuffer->Width, VideoBuffer->Height);

    app_state *State = (app_state *)Memory->PermanentStorageBytes;
    if (!Memory->IsInitialized) {
        Memory->IsInitialized = true;
    }

    for (i32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
        app_controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog) {
            // analog movement tuning
            // I don't have a controller to test this
        }
        else {
        /*
            r32 dValue = 0.0f;
            if (key)
                dValue =  1;
            if (other key)
                dValue = -1;
            dValue *= 100.0f;
            State->Value += Input->dtForFrame * dValue;
         */
        }
    }
    
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES); {
        OpenGLRect(1.0f);
    } glEnd();
}

extern "C" APP_GET_SOUND_SAMPLES(AppGetSoundSamples) {
    app_state *State = (app_state *)Memory->PermanentStorageBytes;
}

