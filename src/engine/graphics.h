#include <windows.h>
#include <gl/gl.h>
//todo: how to get rid of these?

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "lingo.h"
#include "platform.h"
#include "maths.h"
#include "memory.h"

typedef struct _image {
    void *Data;
    i32 w;
    i32 h;
    i32 Format;
} image;

typedef struct _texture {
    u32 Id;
    i32 w;
    i32 h;
    i32 Format;
} texture;

texture TextureFromImage(image Image) {
    texture Result = {0};
    if (Image.Data && Image.w != 0 && Image.h != 0) {
        glGenTextures(1, &Result.Id);
        glBindTexture(GL_TEXTURE_2D, Result.Id);
    }
    Result.w      = Image.w;
    Result.h      = Image.h;
    Result.Format = Image.Format;

    return Result;
}

void gBegin(rv2 Shift, iv2 Size, color4f Color) {
    glLoadIdentity();
    glViewport(Shift.x, Shift.y, Size.w, Size.h);
    glClearColor(Color.r, Color.g, Color.b, Color.a);
    glClear(GL_COLOR_BUFFER_BIT);
    r32 a = 2.0f/Size.w;
    r32 b = 2.0f/Size.h;
    r32 Proj[] = {
         a,  0,  0,  0,
         0, -b,  0,  0,
         0,  0,  1,  0,
        -1,  1,  0,  1
    };
    glLoadMatrixf(Proj);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gDrawRectFromCenter(rv2 Pos, rv2 Size, color4f Color) {
    glBegin(GL_POLYGON); {
        glColor4f(Color.r, Color.g, Color.b, Color.a);
        glVertex2f(Pos.x - Size.w/2.0f, Pos.y - Size.h/2.0f);
        glVertex2f(Pos.x + Size.w/2.0f, Pos.y - Size.h/2.0f);
        glVertex2f(Pos.x + Size.w/2.0f, Pos.y + Size.h/2.0f);
        glVertex2f(Pos.x - Size.w/2.0f, Pos.y + Size.h/2.0f);
    } glEnd();
}

void gDrawLineFromPoints(rv2 a, rv2 b, r32 StrokeWidth, color4f Color) {
    glLineWidth(StrokeWidth);
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINES); {
        glColor4f(Color.r, Color.g, Color.b, Color.a);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(a.x, a.y);
        glVertex2f(b.x, b.y);
    } glEnd();
}

void gDrawRectangleStrokeFromCenter(rv2 Center, rv2 Size, r32 StrokeWidth) {
    glLineWidth(StrokeWidth);
    glBegin(GL_LINE_LOOP); {
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(Center.x - Size.w/2.0f, Center.y - Size.h/2.0f);
        glVertex2f(Center.x + Size.w/2.0f, Center.y - Size.h/2.0f);
        glVertex2f(Center.x + Size.w/2.0f, Center.y + Size.h/2.0f);
        glVertex2f(Center.x - Size.w/2.0f, Center.y + Size.h/2.0f);
    } glEnd();
}

void gDrawFilledCircle(rv2 Center, r32 Radius, u32 IterationCount){
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

void gDrawRectFromTexture(texture Texture,
                          rectf32 SourceRect, 
                          rectf32 DestRect,
                          rv2     Origin,
                          color4f Tint)
{
    f32 w = (f32)Texture.w;
    f32 h = (f32)Texture.h;

    b32 FlipX = 0;

    if (SourceRect.w < 0) {
        FlipX = 0;
        SourceRect.w *= -1;
    }
    if (SourceRect.h < 0) {
        SourceRect.y -= SourceRect.h;
    }

    glBindTexture(GL_TEXTURE_2D, Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS); {
        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
        glNormal3f(0.0f, 0.0f, 1.0f);                          // Normal vector pointing towards viewer

        // Bottom-left corner for texture and quad
        if (FlipX) glTexCoord2f((SourceRect.x + SourceRect.w)/w, SourceRect.y/h);
        else       glTexCoord2f(SourceRect.x/w, SourceRect.y/h);
        glVertex2f(0.0f, 0.0f);

        // Bottom-right corner for texture and quad
        if (FlipX) glTexCoord2f((SourceRect.x + SourceRect.w)/w, (SourceRect.y + SourceRect.h)/h);
        else       glTexCoord2f(SourceRect.x/w, (SourceRect.y + SourceRect.h)/h);
        glVertex2f(0.0f, DestRect.h);

        // Top-right corner for texture and quad
        if (FlipX) glTexCoord2f(SourceRect.x/w, (SourceRect.y + SourceRect.h)/h);
        else       glTexCoord2f((SourceRect.x + SourceRect.w)/w, (SourceRect.y + SourceRect.h)/h);
        glVertex2f(DestRect.w, DestRect.h);

        // Top-left corner for texture and quad
        if (FlipX) glTexCoord2f(SourceRect.x/w, SourceRect.y/h);
        else       glTexCoord2f((SourceRect.x + SourceRect.w)/w, SourceRect.y/h);
        glVertex2f(DestRect.w, 0.0f);
    } glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void gDrawTexture(texture Texture, rv2 Center, rv2 Size, color4f Tint) {
    glBindTexture(GL_TEXTURE_2D, Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS); {
        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
        glTexCoord2f(0, 0);
        glVertex2f  (Center.x - Size.w/2.0f, Center.y - Size.h/2.0f);
        glTexCoord2f(1, 0);
        glVertex2f  (Center.x + Size.w/2.0f, Center.y - Size.h/2.0f);
        glTexCoord2f(1, 1);
        glVertex2f  (Center.x + Size.w/2.0f, Center.y + Size.h/2.0f);
        glTexCoord2f(0, 1);
        glVertex2f  (Center.x - Size.w/2.0f, Center.y + Size.h/2.0f);
    } glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

#include "fonts.h"

void gDrawText(font Font, c8 *Text, rv2 Pos, f32 Size, f32 Spacing, color4f Tint) {
    i32 TextLenght      = ArrayCount(Text);
    f32 ScaleFactor     = Size/Font.Size;
    f32 CharacterOffset = 0;

    glBindTexture(GL_TEXTURE_2D, Font.Texture.Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS); {
        glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);
        i32 i = 0;
        while (Text[i] != '\0') {
            i32 Index     = GetGlyphIndex(Font, Text[i]);
            i32 Codepoint = Font.Chars[Index].Codepoint;
            i32 OffY      = Font.Chars[Index].OffY;
            glColor4f(Tint.r, Tint.g, Tint.b, Tint.a);

            rectf32 Rect = Font.Rects[Index];
            f32 w        = Font.Texture.w;
            f32 h        = Font.Texture.h;

            glTexCoord2f(Rect.x/w, Rect.y/h);
            glVertex2f  (Pos.x + CharacterOffset, Pos.y + OffY);

            glTexCoord2f((Rect.x + Rect.w)/w, Rect.y/h);
            glVertex2f  (Pos.x + CharacterOffset + Rect.w, Pos.y + OffY);

            glTexCoord2f((Rect.x + Rect.w)/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharacterOffset + Rect.w, Pos.y + Rect.h + OffY);

            glTexCoord2f(Rect.x/w, (Rect.y + Rect.h)/h);
            glVertex2f  (Pos.x + CharacterOffset, Pos.y + Rect.h + OffY);

            CharacterOffset += Font.Chars[Index].Advance * ScaleFactor + Spacing;
            i++;
        }
    } glEnd();
    glDisable(GL_TEXTURE_2D);
}

#endif//GRAPHICS_H
