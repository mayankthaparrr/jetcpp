//
// Created by Mayank Thapar on 06-09-2026.
//

#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include "cmake-build-debug/_deps/raylib-src/src/raylib.h"

int main() {
    const char *c ="hello";

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1200, 600, "Jet C++");
    SetTargetFPS(60);
    Color tooblue = GetColor(0x0000aaff);

    //Loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(tooblue);
        DrawRectangle(0,0,1200,30,GRAY);

        EndDrawing();
    }
    return 0;
}


