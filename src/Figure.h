#ifndef TETRIS_FIGURE_H
#define TETRIS_FIGURE_H

#include <cstdio>
#include "bitmap_fonts.h"

struct Figure
{
    int x[4], y[4];
    GLfloat clr[3];
    
    void  print() const
    {
        for (int i=0; i<4; i++)
            printf("x[%1d]=%2d,  y[%1d]=%2d\n", i, x[i], i, y[i]);
    }
    
    void render (int xpos, int ypos, int win[2]) const 
    {
        glColor3fv(clr);
        beginRenderText(-40*8, -32, win[0]-40*8, win[1]-32);

        for (int i=0; i<4; i++)
            renderText(xpos + x[i]*16, ypos + y[i]*16, BITMAP_FONT_TYPE_8_BY_13, (char*) "[]");

        endRenderText();
    }
};

#endif  // TETRIS_FIGURE_H
