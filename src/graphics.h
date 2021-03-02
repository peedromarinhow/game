#include <windows.h>
#include <gl/gl.h>
//todo: how to get rid of these?

#include "lingo.h"
#include "platform.h"
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

typedef struct _bitmap {
    u32  Handle;
    i32  w;
    i32  h;
    u32 *Pixels;
} bitmap;

#pragma pack(push, 1)
typedef struct _bitmap_header {
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    i32 Width;
    i32 Height;
    u16 Planes;
    u16 BitsPerPixel;
} bitmap_header;
#pragma pack(pop)

bitmap LoadBMP(memory_arena *Arena, platform_load_file_callback LoadFile, char *Filename) {
    bitmap Result = {0};
    file BitmapFile = LoadFile(Arena, Filename);
    if (!BitmapFile.Data) {
        //error
    }
    bitmap_header *Header = (bitmap_header *)BitmapFile.Data;
    Result.w      = Header->Width;
    Result.h      = Header->Height;
    Result.Pixels = (u32 *)((u8 *)BitmapFile.Data + Header->BitmapOffset);
    return Result;
}

typedef struct _texture {
    u32   Handle;
    i32   w;
    i32   h;
    void *Pixels;
} texture;

texture GenTextureFromBitmap(bitmap Bitmap) {
    texture Result = {0};
    GLuint TextureHandle = 0;
    static b32 Init = 0;
    glGenTextures(1, &TextureHandle);
    Result.Handle = TextureHandle;
    Result.w      = Bitmap.w;
    Result.h      = Bitmap.h;
    Result.Pixels = Bitmap.Pixels;

    return Result;
}

void DrawTexture(texture Texture, rv2 Center, rv2 Dimensions) {
    // glBindTexture(GL_TEXTURE_2D, Texture.Handle);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, Texture.w, Texture.h, 0,
    //              GL_BGRA_EXT, GL_UNSIGNED_BYTE, Texture.Pixels);
    // glBindTexture(GL_TEXTURE_2D, 0);
    // glEnable(GL_TEXTURE_2D);
    // glBegin(GL_POLYGON); {
    //     glVertex2f(Center.x - Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
    //     glVertex2f(Center.x + Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
    //     glVertex2f(Center.x + Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
    //     glVertex2f(Center.x - Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
    // } glEnd();
    glBindTexture(GL_TEXTURE_2D, Texture.Handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Texture.w, Texture.h, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Texture.Pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glBegin(GL_POLYGON); {
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(Center.x - Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
        glVertex2f(Center.x + Dimensions.w/2.0f, Center.y - Dimensions.h/2.0f);
        glVertex2f(Center.x + Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
        glVertex2f(Center.x - Dimensions.w/2.0f, Center.y + Dimensions.h/2.0f);
    } glEnd();
}
