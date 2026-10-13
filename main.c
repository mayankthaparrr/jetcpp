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
#include "ui/ui_messages.h"
#include "func/toolchain.h"
#include "func/lsp.h"
#include <nfd.h>

float fontsize;
float basefont;
Font ourfont;


int main(int argc, char **argv) {

    //Window Initialization

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1000, 800, "Jet C++");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);


    addLine(0);
    ourfont = LoadFont(TextFormat("%sPx437_IBM_VGA_9x16.ttf", GetApplicationDirectory()));
    if (ourfont.texture.id == 0) ourfont = GetFontDefault();
    if (argc > 1 && argv[1] && argv[1][0] != '\0') {
        loadFileAtPath(argv[1]);
    }

    Font usedfont=ourfont;
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
        toolchainUpdate();
        lspUpdate();
        BeginDrawing();
        ClearBackground(tooblue);
        borders(fontsize,filename,usedfont);
        textstuff(usedfont,fontsize);
        drawMessages(usedfont,fontsize);
        drawRibbon(greyy,usedfont,basefont);
        EndDrawing();

    }
    lspStop();
    NFD_Quit();
    return 0;
}
