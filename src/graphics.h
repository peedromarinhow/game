#include <windows.h>
#include <gl/gl.h>
//todo: how to get rid of these?

#include "lingo.h"
#include "maths.h"
#include "memory.h"

typedef union _color {
    u32 RGBA;
    struct {
        u8 r;
        u8 g;
        u8 b;
        u8 a;
    };
} color;

//todo: (maybe) some wrapper around opengl

void DrawRectangleFromCenter(rv2 Center, rv2 Dimensions) {
    glBegin(GL_POLYGON); {
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(Center.x - Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
        glVertex2f(Center.x + Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
        glVertex2f(Center.x + Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
        glVertex2f(Center.x - Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
    } glEnd();
}

void DrawRectangleStrokeFromCenter(rv2 Center, rv2 Dimensions, r32 StrokeWidth) {
    glLineWidth(StrokeWidth);
    glBegin(GL_LINE_LOOP); {
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(Center.x - Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
        glVertex2f(Center.x + Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
        glVertex2f(Center.x + Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
        glVertex2f(Center.x - Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
    } glEnd();
}

void DrawFilledCircle(rv2 Center, r32 Radius, u32 IterationCount){
	GLfloat TwoPi = 2.0f * PI32;
	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(Center.x, Center.y); // center of circle
		for(u32 IterationIndex = 0; IterationIndex <= IterationCount; IterationIndex++) { 
			glVertex2f(
		        Center.x + (Radius * Cos(IterationIndex * TwoPi / IterationCount)), 
			    Center.y + (Radius * Sin(IterationIndex * TwoPi / IterationCount))
			);
		}
	glEnd();
}

typedef struct _bitmap_texture {
    u32   Handle;
    i32   w;
    i32   h;
    i32   Pitch;
    void *Pixels;
} bitmap_texture;

bitmap_Texture GenTextureFromBitmap(memory_arena *Arena, char *Filename) {
    GLuint TextureHandle = 0;
    static b32 Init = false;
    if (!Init) {
        glGenTextures(1, &TextureHandle);
        Init = true;
    }
}

void DrawTexture(bitmap Texture, rv2 Center, rv2 Dimensions) {
    glBindTexture(GL_TEXTURE_2D, Texture.Handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Texture.w, Texture.h, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Texture.Pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_POLYGON);
        glVertex2f(Center.x - Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
        glVertex2f(Center.x + Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
        glVertex2f(Center.x + Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
        glVertex2f(Center.x - Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
    glEnd();
}
