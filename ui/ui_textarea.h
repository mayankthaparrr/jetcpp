#ifndef ATR_UI_TEXTAREA_H
#define ATR_UI_TEXTAREA_H
#include "raylib.h"
void borders(float,char *,Font);
void textstuff(Font,float);

int addLine(int position);
void showStatusError(const char *msg); // red flash above the bottom ribbon
void caretKeepView(void); // caret moved, but don't scroll to it (Select All)
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
extern float scrollX;
extern float zoom;
extern float lineHeight;
extern Rectangle ScreenRect;
extern Rectangle terminalRect; // the Message pane, under the editor
extern int contentWidthDirty;

#endif