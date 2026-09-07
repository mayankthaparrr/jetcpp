//
// Created by Mayank Thapar on 06-09-2026.
//
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include"ui_ribbon.h"
void drawRibbon(Color,Font,float);
Font Roboto;
float fontsize=30;


int main() {
    //Window Initialization

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1200, 800, "Jet C++");
    SetTargetFPS(60);


    const char *c ="hello";

    Roboto=LoadFont("C:/DEV/ATR/Roboto-Regular.ttf");
    Font usedfont=Roboto;
    Color tooblue = GetColor(0x0000aaff);
    Color greyy = GetColor(0xada8acff);
    char *filename = "NONAME.CPP";
    //Loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(tooblue);
        drawRibbon(greyy,usedfont,fontsize);
        EndDrawing();
    }
    return 0;
}
