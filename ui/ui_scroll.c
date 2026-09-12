//
// Created by Mayank Thapar on 12-09-2026.
//

#include "ui_scroll.h"
#include "raylib.h"


void scrollKeyboard(int cursorLine, float lineHeight, float screenHeight, int *scrollLine) {
    int visibleLines = screenHeight / lineHeight;

    if (cursorLine < *scrollLine)
        *scrollLine = cursorLine;

    if (cursorLine >= *scrollLine + visibleLines)
        *scrollLine = cursorLine - visibleLines + 1;
}

void scrollMouse(int lineCount, int *scrollLine) {
    float wheel = GetMouseWheelMove();

    if (wheel != 0) {
        *scrollLine -= (int)(wheel * 3);

        if (*scrollLine < 0)
            *scrollLine = 0;

        if (*scrollLine > lineCount - 1)
            *scrollLine = lineCount - 1;
    }
}

void scrollHorizontal(float cursorWidth, float screenWidth, float *scrollX) {
    if (cursorWidth - *scrollX > screenWidth)
        *scrollX = cursorWidth - screenWidth;

    if (cursorWidth - *scrollX < 0)
        *scrollX = cursorWidth;

    if (*scrollX < 0)
        *scrollX = 0;
}
