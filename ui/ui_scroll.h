//
// Created by Mayank Thapar on 12-09-2026.
//

#ifndef SCROLL_H
#define SCROLL_H
void scrollKeyboard(int cursorLine, float lineHeight, float screenHeight, int *scrollLine);
void scrollMouse(int lineCount, int *scrollLine);
//void scrollHorizontal(int cursorColumn, float fontSize, float screenWidth, int *scrollColumn);
//void scrollHorizontal(char *buffer, int cursorColumn, Font font, float fontSize, float screenWidth, float startX, float *scrollX);
void scrollHorizontal(float cursorWidth, float screenWidth, float *scrollX);
void scrollbar(float posy,float height,int linecount,float lineheight,int *scrollLine);
void scrollbarHorizontal(float posx,float posy,float width,float contentWidth,float scrollX);
#endif
