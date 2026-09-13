//
// Created by Mayank Thapar on 12-09-2026.
//

#include "ui_scroll.h"
#include "raylib.h"

int draggingScrollbar = 0;
float scrollbarGrabOffset = 0;

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

void scrollbar(float posy,float height,int linecount,float lineheight,int *scrollLine){

    float thumbHeight = (height * (height / lineheight) / linecount);


    if (thumbHeight > height)
        thumbHeight = height;


    float maxScroll = linecount - (height / lineheight)+10;
    float thumbY = posy - 5;
    if (maxScroll > 0)
        thumbY += ((float)*scrollLine / maxScroll) * (height - thumbHeight);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
    CheckCollisionPointRec(GetMousePosition(),(Rectangle){GetScreenWidth()-25,thumbY,20,thumbHeight})) {
        draggingScrollbar = 1;
        scrollbarGrabOffset = GetMouseY() - thumbY;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
    CheckCollisionPointRec(GetMousePosition(),(Rectangle){GetScreenWidth()-25,posy-5,20,height}) &&
    !CheckCollisionPointRec(GetMousePosition(),(Rectangle){GetScreenWidth()-25,thumbY,20,thumbHeight}) &&
    maxScroll > 0) {

        *scrollLine = ((GetMouseY() - (posy - 5)) / height) * maxScroll;
    }

    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        draggingScrollbar = 0;

    if (draggingScrollbar && maxScroll > 0) {
        thumbY = GetMouseY() - scrollbarGrabOffset;

        if (thumbY < posy - 5)
            thumbY = posy - 5;

        if (thumbY > posy - 5 + height - thumbHeight)
            thumbY = posy - 5 + height - thumbHeight;

        *scrollLine = ((thumbY - (posy - 5)) / (height - thumbHeight)) * maxScroll;

        if (*scrollLine < 0)
            *scrollLine = 0;

        if (*scrollLine > maxScroll)
            *scrollLine = maxScroll;
    }

    thumbY = posy - 5;

    if (maxScroll > 0)
        thumbY += ((float)*scrollLine / maxScroll) * (height - thumbHeight);

    if (thumbY < posy - 5)
        thumbY = posy - 5;

    if (thumbY > posy - 5 + height - thumbHeight)
        thumbY = posy - 5 + height - thumbHeight;

    thumbHeight*=1.007;
    Rectangle Column ={GetScreenWidth()-15,posy-5,10,height};
    Rectangle Thumb ={GetScreenWidth()-15,thumbY,10,thumbHeight};

    DrawRectangleRounded(Column,0.4,0,SKYBLUE);
    DrawRectangleRounded(Thumb,0.4,0,WHITE);

}
void scrollbarHorizontal(float posx,float posy,float width,float contentWidth,float scrollX) {
    float thumbWidth = width * width / contentWidth;
    if (thumbWidth > width)
        thumbWidth = width;
    float maxScroll = contentWidth - width;
    float thumbX = posx;

    if (maxScroll > 0)
        thumbX += (scrollX / maxScroll) * (width - thumbWidth);

    Rectangle Column = {posx,posy+10,width,10};
    Rectangle Thumb = {thumbX,posy+10,thumbWidth,10};
    if (thumbWidth < width) {
        DrawRectangleRounded(Column,0.5,0,SKYBLUE);
        DrawRectangleRounded(Thumb,0.5,0,WHITE);
    }
}
