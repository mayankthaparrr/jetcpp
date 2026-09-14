//
// Created by Mayank Thapar on 10-09-2026.
//
#include "ui_ribbonEvents.h"
#include "ui_textarea.h"
#include <stdlib.h>
char* filename;

void newfile() {
    for (int i = 0; i < lineCount; i++) {
        free(lines[i].buffer);
    }

    free(lines);

    lines = NULL;
    lineCount = 0;

    addLine(0);

    cursorLine = 0;
    cursorColumn = 0;

    selectionLine = 0;
    selectionColumn = 0;
    selecting = 0;

    scrollLine = 0;
    scrollColumn = 0;
    scrollX = 0;
}
void openfile() {

}
void savefile() {
    
}
void saveas() {

}
void saveall() {

}