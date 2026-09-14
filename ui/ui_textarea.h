#ifndef ATR_UI_TEXTAREA_H
#define ATR_UI_TEXTAREA_H
#include "raylib.h"
void borders(float,char *,Font);
void textstuff(Font,float);

void addLine(int position);
typedef struct {
    char *buffer;
    int length;
    int capacity;
} Line;

extern Line *lines;
extern int lineCount;
extern int cursorLine;
extern int cursorColumn;
extern int selectionLine;
extern int selectionColumn;
extern int selecting;
extern int scrollLine;
extern int scrollColumn;
extern float scrollX;

#endif