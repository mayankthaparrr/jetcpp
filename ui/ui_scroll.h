//
// Created by Mayank Thapar on 12-09-2026.
//

#ifndef SCROLL_H
#define SCROLL_H
void scrollKeyboard(int cursorLine, float lineHeight, float screenHeight, int *scrollLine);
void scrollMouse(int lineCount, float viewHeight, float lineHeight, int *scrollLine,
                 float viewWidth, float contentWidth, float *scrollX);
//void scrollHorizontal(int cursorColumn, float fontSize, float screenWidth, int *scrollColumn);
//void scrollHorizontal(char *buffer, int cursorColumn, Font font, float fontSize, float screenWidth, float startX, float *scrollX);
void scrollHorizontal(float cursorWidth, float screenWidth, float *scrollX);
// Visibility is decided once per frame by the layout (initrect) with the
// *Needed() tests and passed in, so the text rect and the bars always agree.
int scrollbarNeeded(float height,int linecount,float lineheight);
int scrollbarHorizontalNeeded(float width,float contentWidth);
float scrollbar(float posy,float height,int linecount,float lineheight,int *scrollLine,int visible);
float scrollbarHorizontal(float posx,float posy,float width,float contentWidth,float *scrollX,int visible);
float make_terminal(float ScreenHeight);
#endif
