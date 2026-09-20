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

int addLine(int);

void drawRibbon(Color,Font,float);
void borders(float,char *,Font);
float fontsize;
float basefont; // unzoomed size used for the ribbon; fontsize = basefont * zoom
Font IBM;


int main(int argc, char **argv) {
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
    // Font ships next to the executable (copied by CMake); resolve relative
    // to the exe dir so the app runs from any folder or machine.
    IBM = LoadFont(TextFormat("%sPx437_IBM_VGA_9x16.ttf", GetApplicationDirectory()));
    if (IBM.texture.id == 0) IBM = GetFontDefault(); // missing font must never break drawing

    // Launched with a file argument (e.g. Help -> About opening the README
    // in a new window): load it instead of starting with an empty buffer.
    if (argc > 1 && argv[1] && argv[1][0] != '\0') {
        loadFileAtPath(argv[1]);
    }

    Font usedfont=IBM;
    SetTextureFilter(usedfont.texture, TEXTURE_FILTER_BILINEAR);
    Color tooblue = GetColor(0x0000aaff);
    Color greyy = GetColor(0xada8acff);
    NFD_Init();
    //Loop
    while (!WindowShouldClose() && !quitRequested) {
        if (GetScreenWidth()>1200) {
            basefont=30;
        }
        else {
            basefont=25;
        }
        fontsize=basefont*zoom; // zoom factor set by Ctrl+wheel / Ctrl+'+'
        // (ribbon stays at basefont: menus shouldn't scale with zoom)
        BeginDrawing();
        ClearBackground(tooblue);
        borders(fontsize,filename,usedfont);
        textstuff(usedfont,fontsize);
        drawRibbon(greyy,usedfont,basefont);
        EndDrawing();

    }
    NFD_Quit();
    return 0;
}
