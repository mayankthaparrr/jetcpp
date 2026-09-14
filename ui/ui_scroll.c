//
// Created by Mayank Thapar on 12-09-2026.
//
//this is ui_scroll.c
#include "ui_scroll.h"
#include "raylib.h"
#include "ui_textarea.h" // zoom

int draggingScrollbar = 0;
float scrollbarGrabOffset = 0;
int draggingHorizontalScrollbar = 0;
float horizontalScrollbarGrabOffset = 0;

void scrollKeyboard(int cursorLine, float lineHeight, float screenHeight, int *scrollLine) {
    int visibleLines = screenHeight / lineHeight;

    if (cursorLine < *scrollLine)
        *scrollLine = cursorLine;

    if (cursorLine >= *scrollLine + visibleLines)
        *scrollLine = cursorLine - visibleLines + 1;
}




static float wheelAccumulator = 0.0f;
static float hWheelAccumulator = 0.0f;

void scrollMouse(int lineCount, float viewHeight, float lineHeight, int *scrollLine,
                 float viewWidth, float contentWidth, float *scrollX) {
    float wheel = GetMouseWheelMove();
    if (wheel == 0.0f) {
        return;
    }

    // Ctrl+wheel: zoom in/out. Wheel up = zoom in (standard convention).
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        zoom += wheel * 0.1f;
        if (zoom < 0.5f) zoom = 0.5f;
        if (zoom > 4.0f) zoom = 4.0f;
        return;
    }

    // Shift+wheel scrolls horizontally (in pixels), matching the vertical
    // sign convention: wheel up shows earlier content (scrolls left).
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        hWheelAccumulator += wheel * lineHeight * 3.0f;

        int move = (int)hWheelAccumulator; // truncates toward zero
        hWheelAccumulator -= move;

        *scrollX -= move;

        float maxScroll = contentWidth - viewWidth;
        if (maxScroll < 0) maxScroll = 0;

        if (*scrollX > maxScroll)
            *scrollX = maxScroll;

        if (*scrollX < 0)
            *scrollX = 0;
        return;
    }

    int visibleLines = (int)(viewHeight / lineHeight);
    if (visibleLines < 1) visibleLines = 1;

    // Stop when the last line reaches the bottom of the view — no blank
    // space below the end of the document.
    int maxScroll = lineCount - visibleLines;
    if (maxScroll < 0) maxScroll = 0;

    wheelAccumulator += wheel * 3.0f; // 3 lines per wheel notch

    int move = (int)wheelAccumulator; // truncates toward zero
    wheelAccumulator -= move;

    *scrollLine -= move;
    if (*scrollLine > maxScroll)
        *scrollLine = maxScroll;

    if (*scrollLine < 0)
        *scrollLine = 0;
}

void scrollHorizontal(float cursorWidth, float screenWidth, float *scrollX) {
    screenWidth-=20;
    if (cursorWidth - *scrollX > screenWidth)
        *scrollX = cursorWidth - screenWidth;

    if (cursorWidth - *scrollX < 0)
        *scrollX = cursorWidth;

    if (*scrollX < 0)
        *scrollX = 0;


}

float scrollbar(float posy,float height,int linecount,float lineheight,int *scrollLine){

    float thumbHeight = (height * (height / lineheight) / linecount);


    if (thumbHeight > height)
        thumbHeight = height;


    // Same bound as scrollMouse(): stop when the last line reaches the
    // bottom of the view, so the thumb and wheel agree.
    float maxScroll = linecount - (height / lineheight);
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

    //thumbHeight*=1.007;
    if (thumbHeight<height) {
        Rectangle Column ={GetScreenWidth()-15,posy-5,10,height};
        Rectangle Thumb ={GetScreenWidth()-15,thumbY,10,thumbHeight};
        DrawRectangleRounded(Column,0.5,0,SKYBLUE);
        DrawRectangleRounded(Thumb,0.5,0,WHITE);
        return 15;
    }
    else {
        return 0;
    }

}
float scrollbarHorizontal(float posx,float posy,float width,float contentWidth,float *scrollX) {


    float thumbWidth = width * width / contentWidth;
    if (thumbWidth > width)
        thumbWidth = width;
    float maxScroll = contentWidth - width;
    float thumbX = posx;

    if (maxScroll > 0)
        thumbX += (*scrollX / maxScroll) * (width - thumbWidth);

    Rectangle Column = {posx,posy+10,width,10};
    Rectangle Thumb = {thumbX,posy+10,thumbWidth,10};


    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
    CheckCollisionPointRec(GetMousePosition(), Thumb)) {
        draggingHorizontalScrollbar = 1;
        horizontalScrollbarGrabOffset = GetMouseX() - thumbX;
    }

    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        draggingHorizontalScrollbar = 0;

    if (draggingHorizontalScrollbar && maxScroll > 0) {
        thumbX = GetMouseX() - horizontalScrollbarGrabOffset;

        if (thumbX < posx)
            thumbX = posx;

        if (thumbX > posx + width - thumbWidth)
            thumbX = posx + width - thumbWidth;

        *scrollX = ((thumbX - posx) / (width - thumbWidth)) * maxScroll;
        Thumb.x = thumbX;
    }



    if (thumbWidth < width) {
        DrawRectangleRounded(Column,0.5,0,SKYBLUE);
        DrawRectangleRounded(Thumb,0.5,0,WHITE);
        return 10;
    }
    else {
        return 0;
    }

}
float make_terminal(float ScreenHeight){
    return ScreenHeight*0.4;
}
