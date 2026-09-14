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
float scrollbar(float posy,float height,int linecount,float lineheight,int *scrollLine);
float scrollbarHorizontal(float posx,float posy,float width,float contentWidth,float *scrollX);
float make_terminal(float ScreenHeight);
#endif
