//
// Created by Mayank Thapar on 06-09-2026.
//

#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include "cmake-build-debug/_deps/raylib-src/src/raylib.h"
#include <string.h>


Font ribbonFont;
Font Roboto;

void drawRibbon2();

void drawRibbon3();

int main() {

    const char *c ="hello";

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 800, "Jet C++");
    SetTargetFPS(60);
    Roboto=LoadFont("C:/DEV/ATR/Roboto-Regular.ttf");
    ribbonFont=LoadFont("C:/DEV/ATR/Ac437_IBM_VGA_8x16.ttf");

    Color tooblue = GetColor(0x0000aaff);
    //Loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(tooblue);
        drawRibbon3();
        EndDrawing();
    }
    return 0;
}

void drawRibbon3() {
    int height=(int)GetScreenHeight()*0.04;
    if (GetScreenHeight()<GetScreenWidth()) {
        height=(int)GetScreenWidth()*0.025;
        if (height>(GetScreenHeight())*0.4) {
            height=(int)GetScreenHeight()*0.3;
        }
    }
    printf("%d,%d\n",GetScreenWidth(),GetScreenHeight());
    Color greyy = GetColor(0xada8acff);
    Font usedfont=Roboto;

    // draw ribbon rect
    DrawRectangle(0,0,GetScreenWidth(),height,greyy);

    struct Items {
        char *item;
        int width;
        int height;
    };
    struct Items item[9];
    item[0].item = "File";
    item[1].item = "Edit";
    item[2].item = "Search";
    item[3].item = "Run";
    item[4].item = "Debug";
    item[5].item = "Project";
    item[6].item = "Options";
    item[7].item = "Window";
    item[8].item = "Help";

    int usedwidth=0;
    for (int i=0;i<9;i++) {
        Vector2 textSize=MeasureTextEx(usedfont,item[i].item,height,0);
        item[i].width=textSize.x;
        item[i].height=textSize.y;
        usedwidth+=textSize.x;
    }
    int spaceleft=(int)GetScreenWidth()-usedwidth-(GetScreenWidth()*0.02);
    spaceleft=(int)spaceleft/13;
    int posx=spaceleft*2;

    for (int i=0;i<9;i++) {
        DrawTextEx(usedfont,item[i].item,(Vector2){posx,0},item[i].height,0,BLACK);
        posx=posx+item[i].width+(spaceleft);
        if (i==6) {
            posx+=(spaceleft*3);
        }

    }


}

