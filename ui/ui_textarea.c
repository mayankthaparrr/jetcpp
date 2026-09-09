//
// Created by Mayank Thapar on 10-09-2026.
//

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

Rectangle ScreenRect;
Vector2 fname_size;
void borders(float fontsize,char *filename,Font usedfont) {
    float ribbon_height=fontsize*1.05;
    ScreenRect=(Rectangle){GetScreenWidth()*0.005,ribbon_height*1.4,GetScreenWidth()*0.99,GetScreenHeight()-(ribbon_height*1.4)-GetScreenHeight()*0.01};
    DrawRectangleLinesEx(ScreenRect,2,WHITE);
    ScreenRect.x+=5;
    ScreenRect.y+=5;
    ScreenRect.width-=10;
    ScreenRect.height-=10;
    DrawRectangleLinesEx(ScreenRect,2,WHITE);
    fname_size=MeasureTextEx(usedfont,filename,fontsize,0);
    Color tooblue = GetColor(0x0000aaff);
    DrawRectangle(((GetScreenWidth()-fname_size.x)/2)-8,ribbon_height,fname_size.x+16,fname_size.y,tooblue);
    DrawTextEx(usedfont,filename,(Vector2){(GetScreenWidth()-fname_size.x)/2,ribbon_height+4},fontsize,0,WHITE);

}

int capacity = 8;
char *buffer = NULL;
int length = 0;

void textstuff(Font usedfont, float fontsize) {

    if (buffer == NULL) {
        buffer = malloc(capacity * sizeof(char));
        buffer[0] = '\0';
    }

    int keypressed = GetCharPressed();
    while (keypressed > 0) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity * sizeof(char));
        }
        buffer[length] = (char)keypressed;
        length++;
        buffer[length] = '\0';
        keypressed = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && length > 0) {
        length--;
        buffer[length] = '\0';
    }
    if (IsKeyPressed(KEY_ENTER) && length > 0) {
        length++;
        buffer[length-1] = '\n';
        buffer[length] = '\0';
    }
    BeginScissorMode(ScreenRect.x, ScreenRect.y, ScreenRect.width, ScreenRect.height);
    DrawTextEx(usedfont, buffer, (Vector2){ScreenRect.x*1.8, fname_size.y*2.5}, fontsize, 0, WHITE);
    EndScissorMode();

}

