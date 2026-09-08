//
// Created by Mayank Thapar on 06-09-2026.
//
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include "ui/ui_ribbon.h"
void drawRibbon(Color,Font,float);
Font Roboto;
float fontsize;



int main() {
    //Window Initialization

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1000, 800, "Jet C++");
    SetTargetFPS(60);


    const char *c ="hello";

    Roboto=LoadFont("C:/DEV/ATR/Roboto-Regular.ttf");

    Font usedfont=Roboto;
    SetTextureFilter(usedfont.texture, TEXTURE_FILTER_BILINEAR);
    Color tooblue = GetColor(0x0000aaff);
    Color greyy = GetColor(0xada8acff);
    char *filename = "NONAME.CPP";
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
        DrawLineEx((Vector2){10,fontsize*2.2},(Vector2){20,fontsize*2.2},5,WHITE);
        drawRibbon(greyy,usedfont,fontsize);
        EndDrawing();

    }
    return 0;
}
