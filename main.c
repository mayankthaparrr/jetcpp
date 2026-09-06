//
// Created by Mayank Thapar on 06-09-2026.
//

#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include "cmake-build-debug/_deps/raylib-src/src/raylib.h"

Font ribbonFont;
Font Roboto;
void drawRibbon();
int main() {
    const char *c ="hello";

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1200, 600, "Jet C++");
    SetTargetFPS(60);
    Color tooblue = GetColor(0x0000aaff);
    Roboto=LoadFont("C:/DEV/ATR/Roboto-Regular.ttf");
    ribbonFont=LoadFont("C:/DEV/ATR/Ac437_IBM_VGA_8x16.ttf");

    //Loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(tooblue);
        drawRibbon();
        EndDrawing();
    }
    return 0;
}

void drawRibbon() {

    int screen_width = GetScreenWidth();
    int height = GetScreenHeight();
    Vector2 textSize;


    //scale ribbon height w.r.t screen width
    height=(int)height*0.04;

    //Draw Ribbon BG
    DrawRectangle(0,0,screen_width,height,GRAY);
    //var for pos x of text
    int posx=(int)screen_width*0.01;



    struct Items {
        char *item;
        int width;
    };

    struct Items item[6];
    item[0].item="File";
    item[1].item="Edit";
    item[2].item="Search";
    item[3].item="Run";
    item[4].item="Debug";
    item[5].item="Project";
    item[6].item="Options";

    int totalWidth=0;
    for (int i=0;i<=6;i++) {
        textSize=MeasureTextEx(ribbonFont,item[i].item,height,0);
        item[i].width=textSize.x;
        totalWidth+=textSize.x;
    }
        int padding=(int)screen_width/(totalWidth+5);
        posx=padding;
    for (int i=0;i<=6;i++) {
        //draw text
        posx+=(item[i].width);
        DrawTextEx(ribbonFont,item[i].item,(Vector2){posx,1},height,0,BLACK);
    }









}
