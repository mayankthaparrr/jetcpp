//
// Created by Mayank Thapar on 10-09-2026.
//
//this is ui_textarea.c
//include libs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "ui_scroll.h"

//inits vars
int scrollLine = 0;
int scrollColumn = 0;
float scrollX = 0;
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
int cursorLine = 0;
int cursorColumn = 0;
Line *lines=NULL;
int lineCount=0;
int selectionLine = 0;
int selectionColumn = 0;
int selecting = 0;
float ribbon_height=5;
Rectangle ScreenRect;

//dont ask me bout any of this code. please. i dont know what im doing half the time. thanks.
float vertscroll=0;
float horscroll=0;
float terminal_height=0;
void initrect() {
    ScreenRect=(Rectangle){GetScreenWidth()*0.005,ribbon_height*1.4,GetScreenWidth()*0.99,GetScreenHeight()-ribbon_height*2.8};
    ScreenRect.width-=vertscroll;
    ScreenRect.height-=horscroll;
    ScreenRect.height-=terminal_height;
}

void borders(float fontsize,char *filename,Font usedfont) {
    initrect();
    ribbon_height=fontsize*1.05;
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
double repeatStartTime = 0;
double repeatNextTime = 0;
int repeatKey = 0;

void addLine(int position) {
    lines = realloc(lines, (lineCount + 1) * sizeof(Line));

    memmove(&lines[position + 1], &lines[position], (lineCount - position) * sizeof(Line));

    lines[position].capacity = 16;
    lines[position].length = 0;
    lines[position].buffer = malloc(lines[position].capacity);
    lines[position].buffer[0] = '\0';

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
    float cursorX = startX -scrollX+ MeasureTextEx(usedfont, beforeCursor, fontsize, 0.8).x;
    float cursorY = startY + (cursorLine - scrollLine) * lineHeight;
    DrawTextEx(usedfont, "|", (Vector2){cursorX, cursorY}, fontsize , 0, RED);
}

int keyRepeat(int key) {
    double now = GetTime();

    if (IsKeyPressed(key)) {
        repeatKey = key;
        repeatStartTime = now + 0.4;
        repeatNextTime = now;
        return 1;
    }

    if (repeatKey == key && IsKeyDown(key) && now >= repeatStartTime) {
        if (now >= repeatNextTime) {
            repeatNextTime = now + 0.05;
            return 1;
        }
    }

    return 0;
}

void selection(void) {
    if (selecting && IsKeyPressed(KEY_ESCAPE)) {
        selecting = 0;
        return;
    }
    if (!selecting &&
        (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) &&
        (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
         IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN))) {
        selectionLine = cursorLine;
        selectionColumn = cursorColumn;
        selecting = 1;
    }

    if (!IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) &&
        selectionLine == cursorLine && selectionColumn == cursorColumn) {
        selecting = 0;
    }
}
void drawSelection(Font usedfont, float fontsize) {
    if (!selecting)
        return;

    int startLine = selectionLine;
    int startColumn = selectionColumn;
    int endLine = cursorLine;
    int endColumn = cursorColumn;

    if (startLine > endLine || (startLine == endLine && startColumn > endColumn)) {
        int tempLine = startLine;
        int tempColumn = startColumn;

        startLine = endLine;
        startColumn = endColumn;
        endLine = tempLine;
        endColumn = tempColumn;
    }

    for (int i = startLine; i <= endLine; i++) {
        int lineStart;
        int lineEnd;

        if (i == startLine)
            lineStart = startColumn;
        else
            lineStart = 0;

        if (i == endLine)
            lineEnd = endColumn;
        else
            lineEnd = lines[i].length;

        char beforeStart[lineStart + 1];
        char selected[lineEnd - lineStart + 1];

        memcpy(beforeStart, lines[i].buffer, lineStart);
        beforeStart[lineStart] = '\0';

        memcpy(selected, &lines[i].buffer[lineStart], lineEnd - lineStart);
        selected[lineEnd - lineStart] = '\0';

        float x = startX -scrollX + MeasureTextEx(usedfont, beforeStart, fontsize, 0.8).x;
        float width = MeasureTextEx(usedfont, selected, fontsize, 0.8).x;

        DrawRectangle(x, startY + (i - scrollLine) * lineHeight, width, lineHeight, BLUE); //selection color here
    }
}

void deleteSelection(void) {
    if (!selecting)
        return;

    int startLine = selectionLine;
    int startColumn = selectionColumn;
    int endLine = cursorLine;
    int endColumn = cursorColumn;

    if (startLine > endLine || (startLine == endLine && startColumn > endColumn)) {
        int tempLine = startLine;
        int tempColumn = startColumn;

        startLine = endLine;
        startColumn = endColumn;
        endLine = tempLine;
        endColumn = tempColumn;
    }

    if (startLine == endLine) {
        memmove(&lines[startLine].buffer[startColumn],
                &lines[startLine].buffer[endColumn],
                lines[startLine].length - endColumn + 1);

        lines[startLine].length -= endColumn - startColumn;
    }

    else {
        int firstLength = startColumn;
        int lastLength = lines[endLine].length - endColumn;

        lines[startLine].capacity = firstLength + lastLength + 1;
        lines[startLine].buffer = realloc(lines[startLine].buffer, lines[startLine].capacity);

        memcpy(&lines[startLine].buffer[firstLength],
               &lines[endLine].buffer[endColumn],
               lastLength + 1);

        lines[startLine].length = firstLength + lastLength;

        for (int i = startLine + 1; i <= endLine; i++) {
            free(lines[i].buffer);
        }

        memmove(&lines[startLine + 1],
                &lines[endLine + 1],
                (lineCount - endLine - 1) * sizeof(Line));

        lineCount -= endLine - startLine;
    }

    selecting = 0;
    cursorLine = startLine;
    cursorColumn = startColumn;
}

void copySelection(void) {
    if (!selecting)
        return;

    int startLine = selectionLine;
    int startColumn = selectionColumn;
    int endLine = cursorLine;
    int endColumn = cursorColumn;

    if (startLine > endLine || (startLine == endLine && startColumn > endColumn)) {
        int tempLine = startLine;
        int tempColumn = startColumn;

        startLine = endLine;
        startColumn = endColumn;
        endLine = tempLine;
        endColumn = tempColumn;
    }

    int totalLength = 0;

    for (int i = startLine; i <= endLine; i++) {
        if (i == startLine && i == endLine)
            totalLength += endColumn - startColumn;
        else if (i == startLine)
            totalLength += lines[i].length - startColumn + 1;
        else if (i == endLine)
            totalLength += endColumn + 1;
        else
            totalLength += lines[i].length + 1;
    }

    char *copy = malloc(totalLength + 1);
    int position = 0;

    for (int i = startLine; i <= endLine; i++) {
        int lineStart;
        int lineEnd;

        if (i == startLine)
            lineStart = startColumn;
        else
            lineStart = 0;

        if (i == endLine)
            lineEnd = endColumn;
        else
            lineEnd = lines[i].length;

        memcpy(&copy[position],
               &lines[i].buffer[lineStart],
               lineEnd - lineStart);

        position += lineEnd - lineStart;

        if (i != endLine)
            copy[position++] = '\n';
    }

    copy[position] = '\0';

    SetClipboardText(copy);
    free(copy);
}

void pasteClipboard(void) {
    const char *clipboard = GetClipboardText();

    if (clipboard == NULL || clipboard[0] == '\0')
        return;

    int pasteLength = strlen(clipboard);

    if (strchr(clipboard, '\n') == NULL) {
        Line *line = &lines[cursorLine];

        if (line->length + pasteLength + 1 > line->capacity) {
            line->capacity = line->length + pasteLength + 1;
            line->buffer = realloc(line->buffer, line->capacity);
        }

        memmove(&line->buffer[cursorColumn + pasteLength],
                &line->buffer[cursorColumn],
                line->length - cursorColumn + 1);

        memcpy(&line->buffer[cursorColumn], clipboard, pasteLength);

        line->length += pasteLength;
        cursorColumn += pasteLength;
        return;
    }

    int beforeLength = cursorColumn;
    int afterLength = lines[cursorLine].length - cursorColumn;

    char *before = malloc(beforeLength + 1);
    char *after = malloc(afterLength + 1);

    memcpy(before, lines[cursorLine].buffer, beforeLength);
    before[beforeLength] = '\0';

    memcpy(after, &lines[cursorLine].buffer[cursorColumn], afterLength);
    after[afterLength] = '\0';

    int newLines = 1;

    for (int i = 0; clipboard[i] != '\0'; i++) {
        if (clipboard[i] == '\n')
            newLines++;
    }

    int oldLineCount = lineCount;

    lines = realloc(lines, (lineCount + newLines - 1) * sizeof(Line));

    memmove(&lines[cursorLine + newLines],
            &lines[cursorLine + 1],
            (oldLineCount - cursorLine - 1) * sizeof(Line));

    lineCount += newLines - 1;

    int currentLine = cursorLine;
    const char *start = clipboard;

    for (int i = 0; i < newLines; i++) {
        const char *newline = strchr(start, '\n');
        int length;

        if (newline != NULL)
            length = newline - start;
        else
            length = strlen(start);

        int extra = (i == 0 ? beforeLength : 0) +
                    (i == newLines - 1 ? afterLength : 0);

        lines[currentLine + i].length = length + extra;
        lines[currentLine + i].capacity = lines[currentLine + i].length + 1;
        lines[currentLine + i].buffer = malloc(lines[currentLine + i].capacity);

        int position = 0;

        if (i == 0) {
            memcpy(lines[currentLine + i].buffer, before, beforeLength);
            position += beforeLength;
        }

        memcpy(&lines[currentLine + i].buffer[position], start, length);
        position += length;

        if (i == newLines - 1) {
            memcpy(&lines[currentLine + i].buffer[position], after, afterLength);
            position += afterLength;
        }

        lines[currentLine + i].buffer[position] = '\0';

        if (newline != NULL)
            start = newline + 1;
    }

    free(before);
    free(after);

    cursorLine += newLines - 1;
    cursorColumn = lines[cursorLine].length - afterLength;
}

void navigation(void) {

if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
    IsKeyPressed(KEY_A)) {
    selectionLine = 0;
    selectionColumn = 0;
    cursorLine = lineCount - 1;
    cursorColumn = lines[cursorLine].length;
    selecting = 1;
    return;
}
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
        IsKeyPressed(KEY_C)) {
        copySelection();
        }
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
        IsKeyPressed(KEY_X) && selecting) {
        copySelection();
        deleteSelection();
        return;
    }
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
        IsKeyPressed(KEY_V)) {
        pasteClipboard();
        return;
    }
    if (selecting && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {

        if (IsKeyPressed(KEY_LEFT)) {
            if (selectionLine < cursorLine ||
                (selectionLine == cursorLine && selectionColumn < cursorColumn)) {
                cursorLine = selectionLine;
                cursorColumn = selectionColumn;
            }
            selecting = 0;
            return;
        }

        if (IsKeyPressed(KEY_RIGHT)) {
            if (selectionLine > cursorLine ||
                (selectionLine == cursorLine && selectionColumn > cursorColumn)) {
                cursorLine = selectionLine;
                cursorColumn = selectionColumn;
            }
            selecting = 0;
            return;
        }
    }

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
        scrollKeyboard(cursorLine, lineHeight, ScreenRect.height, &scrollLine);
    }

    if (keyRepeat(KEY_LEFT) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn > 0) {
            cursorColumn--;
        }
        else if (cursorLine > 0) {
            cursorLine--;
            cursorColumn = lines[cursorLine].length;
        }
    }

    if (keyRepeat(KEY_RIGHT) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
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

    if (selecting && IsKeyPressed(KEY_BACKSPACE)) {
        deleteSelection();
        return;
    }

    if (selecting && IsKeyPressed(KEY_DELETE)) {
        deleteSelection();
        return;
    }

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

            //lines[cursorLine - 1].buffer = realloc(lines[cursorLine - 1].buffer,previousLength + currentLength + 1);
            lines[cursorLine - 1].capacity = previousLength + currentLength + 1;
            lines[cursorLine - 1].buffer = realloc(lines[cursorLine - 1].buffer,lines[cursorLine - 1].capacity);
            memcpy(&lines[cursorLine - 1].buffer[previousLength],lines[cursorLine].buffer,currentLength + 1);

            lines[cursorLine - 1].length += currentLength;

            free(lines[cursorLine].buffer);

            memmove(&lines[cursorLine],&lines[cursorLine + 1],(lineCount - cursorLine - 1) * sizeof(Line));

            lineCount--;
            cursorLine--;
            cursorColumn = previousLength;
        }
    }


    if (keyRepeat(KEY_BACKSPACE) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
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
            lines[cursorLine].capacity = currentLength + nextLength + 1;
            lines[cursorLine].buffer = realloc(lines[cursorLine].buffer,lines[cursorLine].capacity);
            memcpy(&lines[cursorLine].buffer[currentLength],lines[cursorLine + 1].buffer,nextLength + 1);
            lines[cursorLine].length += nextLength;
            free(lines[cursorLine + 1].buffer);
            memmove(&lines[cursorLine + 1],&lines[cursorLine + 2],(lineCount - cursorLine - 2) * sizeof(Line));
            lineCount--;
        }
    }
    if (keyRepeat(KEY_DELETE) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn < lines[cursorLine].length) {
            memmove(&lines[cursorLine].buffer[cursorColumn],&lines[cursorLine].buffer[cursorColumn + 1],lines[cursorLine].length - cursorColumn);

            lines[cursorLine].length--;
        }
        else if (cursorLine < lineCount - 1) {
            int currentLength = lines[cursorLine].length;
            int nextLength = lines[cursorLine + 1].length;
            lines[cursorLine].capacity = currentLength + nextLength + 1;
            lines[cursorLine].buffer = realloc(lines[cursorLine].buffer,lines[cursorLine].capacity);
            memcpy(&lines[cursorLine].buffer[currentLength],lines[cursorLine + 1].buffer,nextLength + 1);
            lines[cursorLine].length += nextLength;
            free(lines[cursorLine + 1].buffer);
            memmove(&lines[cursorLine + 1],&lines[cursorLine + 2],(lineCount - cursorLine - 2) * sizeof(Line));
            lineCount--;
        }
    }
}

void textstuff(Font usedfont, float fontsize) {
    selection();

    float contentWidth = 0;

    for (int i = 0; i < lineCount; i++) {
        float width = MeasureTextEx(usedfont,lines[i].buffer,fontsize,0.8).x;

        if (width > contentWidth)
            contentWidth = width;
    }
    horscroll=scrollbarHorizontal(startX,ScreenRect.y + ScreenRect.height,ScreenRect.width-5,contentWidth,&scrollX);


    BeginScissorMode(ScreenRect.x, ScreenRect.y+20, ScreenRect.width-3, ScreenRect.height-20);

    navigation();

    scrollMouse(lineCount, &scrollLine);
    drawSelection(usedfont, fontsize);
    for (int i = 0; i < lineCount; i++) {
        DrawTextEx(usedfont,lines[i].buffer,(Vector2){startX-scrollX,startY + (i - scrollLine) * lineHeight},fontsize,0.8,WHITE);

    }


    int keypressed = GetCharPressed();
    while (keypressed > 0) {
        if (selecting && keypressed > 0) {
            deleteSelection();
        }
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
    // Hello World
    if (IsKeyPressed(KEY_ENTER)) {
        int remainingLength = lines[cursorLine].length - cursorColumn;
        addLine(cursorLine + 1);
        if (remainingLength + 1 > lines[cursorLine + 1].capacity) {
            lines[cursorLine + 1].capacity = remainingLength + 1;
            lines[cursorLine + 1].buffer = realloc(lines[cursorLine + 1].buffer,lines[cursorLine + 1].capacity);
        }

        memmove(lines[cursorLine + 1].buffer,&lines[cursorLine].buffer[cursorColumn],remainingLength + 1);
        lines[cursorLine + 1].length = remainingLength;
        lines[cursorLine].buffer[cursorColumn] = '\0';
        lines[cursorLine].length = cursorColumn;
        cursorLine++;
        cursorColumn = 0;
    }
    if (IsKeyPressed(KEY_TAB)) {
        Line *line = &lines[cursorLine];
        if (line->length + 1 >= line->capacity) {
            line->capacity *= 2;
            line->buffer = realloc(line->buffer, line->capacity);
        }

        memmove(&line->buffer[cursorColumn + 1],&line->buffer[cursorColumn],line->length - cursorColumn + 1);
        line->buffer[cursorColumn] = '\t';
        line->length++;
        cursorColumn++;
        line->buffer[line->length] = '\0';
    }
    char beforeCursor[lines[cursorLine].capacity];
    memcpy(beforeCursor, lines[cursorLine].buffer, cursorColumn);
    beforeCursor[cursorColumn] = '\0';

    float cursorWidth = MeasureTextEx(usedfont, beforeCursor, fontsize, 0.8).x;
    if (GetKeyPressed() != 0) {
    scrollHorizontal(cursorWidth, ScreenRect.width, &scrollX);
        // some key was pressed
    }
    cursor(usedfont, fontsize, startX, startY, lineHeight);
    EndScissorMode();
    vertscroll=scrollbar(ScreenRect.y,ScreenRect.height,lineCount,lineHeight,&scrollLine);
    //terminal_height=make_terminal(ScreenRect.height);
}


//Todo shift+home, shift+end
