//
// Created by Mayank Thapar on 10-09-2026.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

Rectangle ScreenRect;
Vector2 fname_size;
int capacity = 16;
char *buffer = NULL;
int length = 0;
int cursorPos = 0;
Vector2 textpos;
float cursorX, cursorY;
Font usedfont;

typedef struct {
    char *buffer;
    int length;
    int capacity;
} Line;
float startX ;
float startY ;
float lineHeight ;
float fonttsize;


void borders(float fontsize,char *filename,Font usedfont) {
     float ribbon_height=fontsize*1.05;
     ScreenRect=(Rectangle){GetScreenWidth()*0.005,ribbon_height*1.4,GetScreenWidth()*0.99,GetScreenHeight()-(ribbon_height*1.4)-GetScreenHeight()*0.01};
     DrawRectangleLinesEx(ScreenRect,2,WHITE);
     ScreenRect.x+=5;
     ScreenRect.y+=5;
     ScreenRect.width-=10;
     ScreenRect.height-=10;
     DrawRectangleLinesEx(ScreenRect,2,WHITE);
     fname_size=MeasureTextEx(usedfont,filename,fontsize,0);
     Color tooblue = GetColor(0x0000aaff);
     DrawRectangle(((GetScreenWidth()-fname_size.x)/2)-8,ribbon_height,fname_size.x+16,fname_size.y,tooblue);
     DrawTextEx(usedfont,filename,(Vector2){(GetScreenWidth()-fname_size.x)/2,ribbon_height+4},fontsize,0,WHITE);
     startX = ScreenRect.x * 1.8f;
     startY = fname_size.y * 2.5f;
     lineHeight = MeasureTextEx(usedfont, "Jet", fontsize, 0).y;
     fonttsize=fontsize;
}
int cursorLine = 0;
int cursorColumn = 0;
Line *lines=NULL;
int lineCount=0;

void addLine(void) {
    lines = realloc(lines, (lineCount + 1) * sizeof(Line));
    lines[lineCount].capacity = 16;
    lines[lineCount].length = 0;
    lines[lineCount].buffer = malloc(lines[lineCount].capacity);
    lines[lineCount].buffer[0] = '\0';
    lineCount++;
}

int isWordChar(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_';
}

void cursor(Font usedfont, float fontsize, float startX, float startY, float lineHeight) {
    char beforeCursor[lines[cursorLine].capacity];
    memcpy(beforeCursor, lines[cursorLine].buffer, cursorColumn);
    beforeCursor[cursorColumn] = '\0';
    float cursorX = startX + MeasureTextEx(usedfont, beforeCursor, fontsize, 0.8).x;
    float cursorY = startY + cursorLine * lineHeight;
    DrawTextEx(usedfont, "|", (Vector2){cursorX, cursorY}, fontsize , 0, RED);
}

void navigation(void) {

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_LEFT)) {
        while (cursorColumn > 0 && !isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
            cursorColumn--;

        while (cursorColumn > 0 && isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
            cursorColumn--;

        if (cursorColumn == 0 && cursorLine > 0) {
            cursorLine--;
            cursorColumn = lines[cursorLine].length;
        }
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_RIGHT)) {
        while (cursorColumn < lines[cursorLine].length && !isWordChar(lines[cursorLine].buffer[cursorColumn]))
            cursorColumn++;

        while (cursorColumn < lines[cursorLine].length && isWordChar(lines[cursorLine].buffer[cursorColumn]))
            cursorColumn++;
        if (cursorColumn == lines[cursorLine].length && cursorLine < lineCount - 1) {
            cursorLine++;
            cursorColumn = 0;
        }

    }

    if (IsKeyPressed(KEY_LEFT) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn > 0) {
            cursorColumn--;
        }
        else if (cursorLine > 0) {
            cursorLine--;
            cursorColumn = lines[cursorLine].length;
        }
    }

    if (IsKeyPressed(KEY_RIGHT) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn < lines[cursorLine].length) {
            cursorColumn++;
        }
        else if (cursorLine < lineCount - 1) {
            cursorLine++;
            cursorColumn = 0;
        }
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_UP)) {
        cursorLine = 0;
        cursorColumn = 0;
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_DOWN)) {
        cursorLine = lineCount - 1;
        cursorColumn = lines[cursorLine].length;
    }

    if (IsKeyPressed(KEY_UP) && cursorLine > 0) {
        cursorLine--;

        if (cursorColumn > lines[cursorLine].length)
            cursorColumn = lines[cursorLine].length;
    }

    if (IsKeyPressed(KEY_DOWN) && cursorLine < lineCount - 1) {
        cursorLine++;

        if (cursorColumn > lines[cursorLine].length)
            cursorColumn = lines[cursorLine].length;
    }

    if (IsKeyPressed(KEY_HOME))
        cursorColumn = 0;

    if (IsKeyPressed(KEY_END))
        cursorColumn = lines[cursorLine].length;

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_BACKSPACE)) {
        int oldColumn = cursorColumn;
        while (cursorColumn > 0 && !isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
            cursorColumn--;

        while (cursorColumn > 0 && isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
            cursorColumn--;

        memmove(&lines[cursorLine].buffer[cursorColumn],&lines[cursorLine].buffer[oldColumn],lines[cursorLine].length - oldColumn + 1);
        lines[cursorLine].length -= oldColumn - cursorColumn;
        if (cursorColumn == 0 && cursorLine > 0) {
            int previousLength = lines[cursorLine - 1].length;
            int currentLength = lines[cursorLine].length;

            lines[cursorLine - 1].buffer = realloc(
                lines[cursorLine - 1].buffer,
                previousLength + currentLength + 1
            );

            memcpy(
                &lines[cursorLine - 1].buffer[previousLength],
                lines[cursorLine].buffer,
                currentLength + 1
            );

            lines[cursorLine - 1].length += currentLength;

            free(lines[cursorLine].buffer);

            memmove(
                &lines[cursorLine],
                &lines[cursorLine + 1],
                (lineCount - cursorLine - 1) * sizeof(Line)
            );

            lineCount--;
            cursorLine--;
            cursorColumn = previousLength;
        }
    }


    if (IsKeyPressed(KEY_BACKSPACE) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn > 0) {
            memmove(&lines[cursorLine].buffer[cursorColumn - 1],&lines[cursorLine].buffer[cursorColumn],lines[cursorLine].length - cursorColumn + 1);

            cursorColumn--;
            lines[cursorLine].length--;
        }

        else if (cursorColumn == 0 && cursorLine > 0) {
            int previousLength = lines[cursorLine - 1].length;
            int currentLength = lines[cursorLine].length;

            lines[cursorLine - 1].capacity = previousLength + currentLength + 1;
            lines[cursorLine - 1].buffer = realloc(lines[cursorLine - 1].buffer,lines[cursorLine - 1].capacity);

            memcpy(&lines[cursorLine - 1].buffer[previousLength],lines[cursorLine].buffer,currentLength + 1);

            lines[cursorLine - 1].length += currentLength;
            cursorLine--;
            cursorColumn = previousLength;

            free(lines[cursorLine + 1].buffer);

            memmove(&lines[cursorLine + 1],&lines[cursorLine + 2],(lineCount - cursorLine - 2) * sizeof(Line));

            lineCount--;
        }
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_DELETE)) {
        int oldColumn = cursorColumn;
        while (cursorColumn < lines[cursorLine].length && !isWordChar(lines[cursorLine].buffer[cursorColumn]))
            cursorColumn++;

        while (cursorColumn < lines[cursorLine].length && isWordChar(lines[cursorLine].buffer[cursorColumn]))
            cursorColumn++;
        memmove(&lines[cursorLine].buffer[oldColumn],&lines[cursorLine].buffer[cursorColumn],lines[cursorLine].length - cursorColumn + 1);

        lines[cursorLine].length -= cursorColumn - oldColumn;
        cursorColumn = oldColumn;

        if (cursorColumn == lines[cursorLine].length && cursorLine < lineCount - 1) {
            int currentLength = lines[cursorLine].length;
            int nextLength = lines[cursorLine + 1].length;

            lines[cursorLine].buffer = realloc(
                lines[cursorLine].buffer,
                currentLength + nextLength + 1
            );

            memcpy(
                &lines[cursorLine].buffer[currentLength],
                lines[cursorLine + 1].buffer,
                nextLength + 1
            );

            lines[cursorLine].length += nextLength;

            free(lines[cursorLine + 1].buffer);

            memmove(
                &lines[cursorLine + 1],
                &lines[cursorLine + 2],
                (lineCount - cursorLine - 2) * sizeof(Line)
            );

            lineCount--;
        }
    }
    if (IsKeyPressed(KEY_DELETE) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn < lines[cursorLine].length) {
            memmove(&lines[cursorLine].buffer[cursorColumn],&lines[cursorLine].buffer[cursorColumn + 1],lines[cursorLine].length - cursorColumn);

            lines[cursorLine].length--;
        }
        else if (cursorLine < lineCount - 1) {
            int currentLength = lines[cursorLine].length;
            int nextLength = lines[cursorLine + 1].length;

            lines[cursorLine].buffer = realloc(lines[cursorLine].buffer,currentLength + nextLength + 1);

            memcpy(
                &lines[cursorLine].buffer[currentLength],lines[cursorLine + 1].buffer,nextLength + 1);

            lines[cursorLine].length += nextLength;

            free(lines[cursorLine + 1].buffer);

            memmove(&lines[cursorLine + 1],&lines[cursorLine + 2],(lineCount - cursorLine - 2) * sizeof(Line));

            lineCount--;
        }
    }



}

void textstuff(Font usedfont, float fontsize) {



    BeginScissorMode(ScreenRect.x, ScreenRect.y, ScreenRect.width, ScreenRect.height);
    navigation();
    for (int i = 0; i < lineCount; i++) {

        DrawTextEx(usedfont,lines[i].buffer,(Vector2){startX,startY + i * lineHeight},fontsize,0.8,WHITE);

    }


    int keypressed = GetCharPressed();

    while (keypressed > 0) {
        Line *line = &lines[cursorLine];

        if (line->length + 1 >= line->capacity) {
            line->capacity *= 2;
            line->buffer = realloc(line->buffer, line->capacity);
        }
        memmove(&line->buffer[cursorColumn + 1],&line->buffer[cursorColumn],line->length - cursorColumn + 1);
        line->buffer[cursorColumn] = (char)keypressed;

        line->length++;
        cursorColumn++;

        line->buffer[line->length] = '\0';

        keypressed = GetCharPressed();
    }
    if (IsKeyPressed(KEY_ENTER)) {
        int remainingLength = lines[cursorLine].length - cursorColumn;

        addLine();

        if (remainingLength + 1 > lines[cursorLine + 1].capacity) {
            lines[cursorLine + 1].capacity = remainingLength + 1;
            lines[cursorLine + 1].buffer = realloc(
                lines[cursorLine + 1].buffer,
                lines[cursorLine + 1].capacity
            );
        }

        memmove(lines[cursorLine + 1].buffer,
                &lines[cursorLine].buffer[cursorColumn],
                remainingLength + 1);

        lines[cursorLine + 1].length = remainingLength;

        lines[cursorLine].buffer[cursorColumn] = '\0';
        lines[cursorLine].length = cursorColumn;
        cursorLine++;
        cursorColumn = 0;
    }
    cursor(usedfont, fontsize, startX, startY, lineHeight);
    EndScissorMode();
}
