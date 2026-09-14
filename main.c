//
// Created by Mayank Thapar on 06-09-2026.
//
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include "ui/ui_ribbon.h"
#include "ui/ui_textarea.h"
#include "ui/ui_ribbonEvents.h"
#include <nfd.h>

void addLine(int);

void drawRibbon(Color,Font,float);
void borders(float,char *,Font);
Font Roboto;
float fontsize;
Font IBM;


int main() {
#ifdef _WIN32
    //SetConfigFlags(FLAG_WINDOW_UNDECORATED);
#endif
    //Todo: Custom Window Title Bar
    //
    //Window Initialization

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1000, 800, "Jet C++");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);


    addLine(0);
    Roboto=LoadFont("C:/DEV/ATR/Roboto-Regular.ttf");
    IBM=LoadFont("C:/DEV/ATR/Px437_IBM_VGA_9x16.ttf");

    Font usedfont=IBM;
    SetTextureFilter(usedfont.texture, TEXTURE_FILTER_BILINEAR);
    Color tooblue = GetColor(0x0000aaff);
    Color greyy = GetColor(0xada8acff);
    NFD_Init();
    //Loop
    while (!WindowShouldClose()) {
        if (GetScreenWidth()>1200) {
            fontsize=30;
        }
        else {
            fontsize=25;
        }
        BeginDrawing();
        ClearBackground(tooblue);
        borders(fontsize,filename,usedfont);
        textstuff(usedfont,fontsize);
        drawRibbon(greyy,usedfont,fontsize);
        EndDrawing();

    }
    NFD_Quit();
    return 0;
}
