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
#include "../func/undo.h"

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

// Render-clip state (refreshed every frame in textstuff()):
// charW = monospace advance per column, used instead of MeasureTextEx everywhere.
// cachedContentWidth feeds the horizontal scrollbar; only rescanned on edits.
static float charW = 10.0f;
static float cachedContentWidth = 0;
int contentWidthDirty = 1;

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
float zoom = 1.0f; // font size multiplier (Ctrl+wheel / Ctrl+'+')
float ribbon_height=5;
Rectangle ScreenRect;

// ---------------------------------------------------------------------------
// OOM discipline (K5 fix): realloc failure must degrade to "edit refused",
// never leak the old block or deref NULL. All growth goes through these.
static int oomDrillCountdown = 0;   // OOM drill: fails the Nth grow when armed

// Shared error-status flash (drawn above the bottom ribbon by textstuff()).
// One mechanism for every refused/failed operation: OOM, save failure, ...
static char statusMsg[80] = "";
static double statusUntil = 0;

void showStatusError(const char *msg) {
    snprintf(statusMsg, sizeof statusMsg, "%s", msg);
    statusUntil = GetTime() + 2.0;
}

static void oomRefuse(void) {
    showStatusError("OUT OF MEMORY - edit refused");
}

// Grow one line's text buffer. Returns 1 on success; on failure the line is
// left untouched (old buffer valid) and the caller must abort the edit.
static int growLine(Line *line, int newCapacity) {
    if (newCapacity <= line->capacity)
        return 1; // already fits
    if (oomDrillCountdown > 0 && --oomDrillCountdown == 0) {
        oomRefuse();
        return 0;
    }
    char *grown = realloc(line->buffer, newCapacity);
    if (!grown) {
        oomRefuse();
        return 0;
    }
    line->buffer = grown;
    line->capacity = newCapacity;
    return 1;
}

// Grow the global lines[] array. Same contract as growLine.
static int growLines(int newCount) {
    if (oomDrillCountdown > 0 && --oomDrillCountdown == 0) {
        oomRefuse();
        return 0;
    }
    Line *grown = realloc(lines, newCount * sizeof(Line));
    if (!grown) {
        oomRefuse();
        return 0;
    }
    lines = grown;
    return 1;
}
// ---------------------------------------------------------------------------

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

int addLine(int position) {
    if (!growLines(lineCount + 1)) {
        return 0; // OOM: line array unchanged, caller aborts
    }

    memmove(&lines[position + 1], &lines[position], (lineCount - position) * sizeof(Line));

    lines[position].capacity = 16;
    lines[position].length = 0;
    lines[position].buffer = malloc(lines[position].capacity);
    if (!lines[position].buffer) {
        // Garbage slot at [lineCount] is beyond lineCount (never rendered,
        // overwritten by the next addLine) — safe to refuse.
        oomRefuse();
        return 0;
    }
    lines[position].buffer[0] = '\0';

    lineCount++;
    return 1;
}

int isWordChar(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_';
}

void cursor(Font usedfont, float fontsize, float startX, float startY, float lineHeight) {
    int relLine = cursorLine - scrollLine;
    if (relLine < 0 || relLine > (int)(ScreenRect.height / lineHeight) + 2)
        return; // offscreen: skip work, scissor would hide it anyway
    float cursorX = startX -scrollX + cursorColumn * charW;
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
        !IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        selectionLine == cursorLine && selectionColumn == cursorColumn) {
        selecting = 0;
    }
}
void posFromMouse(Font usedfont, float fontsize, Vector2 mouse, int *line, int *col) {
    *line = scrollLine + (int)((mouse.y - startY) / lineHeight);

    if (*line < 0) {
        *line = 0;
        *col = 0;
        return;
    }

    if (*line >= lineCount) {
        *line = lineCount - 1;
        *col = lines[*line].length;
        return;
    }

    float x = mouse.x - startX + scrollX;
    int c = 0;

    while (c < lines[*line].length) {
        char save = lines[*line].buffer[c + 1];
        lines[*line].buffer[c + 1] = '\0';
        float w = MeasureTextEx(usedfont, lines[*line].buffer, fontsize, 0.8).x;
        lines[*line].buffer[c + 1] = save;

        if (w > x)
            break;

        c++;
    }

    *col = c;
}

void mouseSelection(Font usedfont, float fontsize) {
    static int dragging = 0;
    static int dragMoved = 0;

    Vector2 mouse = GetMousePosition();

    if (mouse.x < ScreenRect.x || mouse.x > ScreenRect.x + ScreenRect.width ||
        mouse.y < ScreenRect.y || mouse.y > ScreenRect.y + ScreenRect.height)
        return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int line;
        int column;

        posFromMouse(usedfont, fontsize, mouse, &line, &column);

        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            if (!selecting) {
                selectionLine = cursorLine;
                selectionColumn = cursorColumn;
            }
            selecting = 1;
        }
        else {
            selectionLine = line;
            selectionColumn = column;
            dragMoved = 0;
            selecting = 1;
        }

        cursorLine = line;
        cursorColumn = column;
        dragging = 1;
    }
    else if (dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        int line;
        int column;

        posFromMouse(usedfont, fontsize, mouse, &line, &column);

        if (line != cursorLine || column != cursorColumn) {
            cursorLine = line;
            cursorColumn = column;
            dragMoved = 1;
        }
    }
    else if (dragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        dragging = 0;

        // a plain click (no drag) collapses the selection
        if (!dragMoved && selectionLine == cursorLine && selectionColumn == cursorColumn)
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

    // Intersect the selection with the visible range: offscreen lines draw
    // nothing instead of paying VLA + MeasureTextEx cost.
    int first = scrollLine;
    if (first < 0) first = 0;
    int last = scrollLine + (int)(ScreenRect.height / lineHeight) + 2;
    if (last > lineCount) last = lineCount;
    int from = startLine > first ? startLine : first;
    int to = endLine < last ? endLine : last;

    for (int i = from; i <= to; i++) {
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

        // Monospace: column * advance, no measuring, no VLAs.
        float x = startX -scrollX + lineStart * charW;
        float width = (lineEnd - lineStart) * charW;

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
        contentWidthDirty = 1;
    }

    else {
        int firstLength = startColumn;
        int lastLength = lines[endLine].length - endColumn;

        if (!growLine(&lines[startLine], firstLength + lastLength + 1)) {
            return; // OOM: refuse the delete, selection stays intact
        }

        memcpy(&lines[startLine].buffer[firstLength],
               &lines[endLine].buffer[endColumn],
               lastLength + 1);

        lines[startLine].length = firstLength + lastLength;
        contentWidthDirty = 1;

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
    if (!copy) {
        showStatusError("OUT OF MEMORY - copy failed");
        return;
    }
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

        if (line->length + pasteLength + 1 > line->capacity &&
            !growLine(line, line->length + pasteLength + 1)) {
            return; // OOM: paste refused, buffer untouched
        }

        memmove(&line->buffer[cursorColumn + pasteLength],
                &line->buffer[cursorColumn],
                line->length - cursorColumn + 1);

        memcpy(&line->buffer[cursorColumn], clipboard, pasteLength);

        line->length += pasteLength;
        contentWidthDirty = 1;
        cursorColumn += pasteLength;
        return;
    }

    int beforeLength = cursorColumn;
    int afterLength = lines[cursorLine].length - cursorColumn;

    char *before = malloc(beforeLength + 1);
    char *after = malloc(afterLength + 1);

    if (!before || !after) {
        free(before);
        free(after);
        showStatusError("OUT OF MEMORY - paste refused");
        return;
    }

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

    if (!growLines(lineCount + newLines - 1)) {
        free(before); // OOM: refuse the multi-line paste entirely
        free(after);
        return;
    }

    memmove(&lines[cursorLine + newLines],
            &lines[cursorLine + 1],
            (oldLineCount - cursorLine - 1) * sizeof(Line));

    // The slot at cursorLine still holds the old line's buffer; free it
    // before the loop below replaces it with fresh allocations.
    free(lines[cursorLine].buffer);

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

        // Strip a trailing carriage return (Windows CRLF clipboard text)
        // so it doesn't end up inside the line buffer.
        while (length > 0 && start[length - 1] == '\r')
            length--;

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

    contentWidthDirty = 1;
    cursorLine += newLines - 1;
    cursorColumn = lines[cursorLine].length - afterLength;
}

void navigation(void) {

    // Any caret move finalizes a coalescing run (VsCode closes runs on move).
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP) ||
        IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_HOME) || IsKeyPressed(KEY_END))
        undo_push(1);

    // Undo / redo: Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z (mirrors office editors).
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_Z) &&
            !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
            undo_undo();
            return;
        }
        if (IsKeyPressed(KEY_Y) ||
            (IsKeyPressed(KEY_Z) &&
             (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))) {
            undo_redo();
            return;
        }
    }

    // Ctrl+'+' (main-row '=' or keypad '+'): zoom in.
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
        (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))) {
        zoom *= 1.1f;
        if (zoom > 4.0f) zoom = 4.0f;
    }

    // Ctrl+'-' (main-row or keypad): zoom out.
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
        (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))) {
        zoom /= 1.1f;
        if (zoom < 0.5f) zoom = 0.5f;
    }

    // Ctrl+0 (main-row or keypad): reset zoom to 100%.
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
        (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0))) {
        zoom = 1.0f;
    }

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
        undo_run_selection();
        deleteSelection();
        undo_push(1);
        return;
    }
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
        IsKeyPressed(KEY_V)) {
        if (selecting) {
            undo_run_selection(); // one step: paste swallows the selection
            deleteSelection();
        } else {
            undo_discrete(cursorLine, 1);
        }
        int beforeLineCount = lineCount;
        pasteClipboard();
        undo_push(lineCount - beforeLineCount + 1);
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

    if (keyRepeat(KEY_UP) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL) && cursorLine > 0) {
        cursorLine--;

        if (cursorColumn > lines[cursorLine].length)
            cursorColumn = lines[cursorLine].length;


            scrollKeyboard(cursorLine, lineHeight, ScreenRect.height, &scrollLine);
    }

    if (keyRepeat(KEY_DOWN) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL) && cursorLine < lineCount - 1) {
        cursorLine++;

        if (cursorColumn > lines[cursorLine].length)
            cursorColumn = lines[cursorLine].length;

        scrollKeyboard(cursorLine, lineHeight, ScreenRect.height-40, &scrollLine);
    }

    if (IsKeyPressed(KEY_HOME)) {
        cursorColumn = 0;
        cursorLine = 0;
        scrollKeyboard(cursorLine, lineHeight, ScreenRect.height-40, &scrollLine);
    }



    if (IsKeyPressed(KEY_END)) {
        cursorColumn = lines[cursorLine].length;
        scrollKeyboard(cursorLine, lineHeight, ScreenRect.height-40, &scrollLine);
    }
    if (selecting && IsKeyPressed(KEY_BACKSPACE)) {
        undo_run_selection();
        deleteSelection();
        undo_push(1);
        return;
    }

    if (selecting && IsKeyPressed(KEY_DELETE)) {
        undo_run_selection();
        deleteSelection();
        undo_push(1);
        return;
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_BACKSPACE)) {
        int tmpCol = cursorColumn;
        while (tmpCol > 0 && !isWordChar(lines[cursorLine].buffer[tmpCol - 1])) tmpCol--;
        while (tmpCol > 0 && isWordChar(lines[cursorLine].buffer[tmpCol - 1])) tmpCol--;
        int mergeUp = (tmpCol == 0 && cursorLine > 0);
        undo_discrete(mergeUp ? cursorLine - 1 : cursorLine, mergeUp ? 2 : 1);

        int oldColumn = cursorColumn;
        while (cursorColumn > 0 && !isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
            cursorColumn--;

        while (cursorColumn > 0 && isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
            cursorColumn--;

        memmove(&lines[cursorLine].buffer[cursorColumn],&lines[cursorLine].buffer[oldColumn],lines[cursorLine].length - oldColumn + 1);
        lines[cursorLine].length -= oldColumn - cursorColumn;
        contentWidthDirty = 1;
        if (cursorColumn == 0 && cursorLine > 0) {
            int previousLength = lines[cursorLine - 1].length;
            int currentLength = lines[cursorLine].length;

            if (!growLine(&lines[cursorLine - 1], previousLength + currentLength + 1)) {
                undo_push(2); // OOM: word delete above still stands, merge refused
                return;
            }
            memcpy(&lines[cursorLine - 1].buffer[previousLength],lines[cursorLine].buffer,currentLength + 1);

            lines[cursorLine - 1].length += currentLength;
            contentWidthDirty = 1;

            free(lines[cursorLine].buffer);

            memmove(&lines[cursorLine],&lines[cursorLine + 1],(lineCount - cursorLine - 1) * sizeof(Line));

            lineCount--;
            cursorLine--;
            cursorColumn = previousLength;
        }
        undo_push(1);
    }


    if (keyRepeat(KEY_BACKSPACE) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn > 0) {
            undo_run_line(); // coalesce char backspaces on a line
            memmove(&lines[cursorLine].buffer[cursorColumn - 1],&lines[cursorLine].buffer[cursorColumn],lines[cursorLine].length - cursorColumn + 1);

            cursorColumn--;
            lines[cursorLine].length--;
            contentWidthDirty = 1;
        }

        else if (cursorColumn == 0 && cursorLine > 0) {
            undo_discrete(cursorLine - 1, 2); // close the run; merge is discrete
            int previousLength = lines[cursorLine - 1].length;
            int currentLength = lines[cursorLine].length;

            if (!growLine(&lines[cursorLine - 1], previousLength + currentLength + 1)) {
                undo_push(2); // OOM: merge refused, nothing deleted this frame
                return;
            }

            memcpy(&lines[cursorLine - 1].buffer[previousLength],lines[cursorLine].buffer,currentLength + 1);

            lines[cursorLine - 1].length += currentLength;
            contentWidthDirty = 1;
            cursorLine--;
            cursorColumn = previousLength;

            free(lines[cursorLine + 1].buffer);

            memmove(&lines[cursorLine + 1],&lines[cursorLine + 2],(lineCount - cursorLine - 2) * sizeof(Line));

            lineCount--;
            undo_push(1);
        }
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_DELETE)) {
        int tmpCol = cursorColumn;
        while (tmpCol < lines[cursorLine].length && !isWordChar(lines[cursorLine].buffer[tmpCol])) tmpCol++;
        while (tmpCol < lines[cursorLine].length && isWordChar(lines[cursorLine].buffer[tmpCol])) tmpCol++;
        int mergeDown = (tmpCol == lines[cursorLine].length && cursorLine < lineCount - 1);
        undo_discrete(cursorLine, mergeDown ? 2 : 1);

        int oldColumn = cursorColumn;
        while (cursorColumn < lines[cursorLine].length && !isWordChar(lines[cursorLine].buffer[cursorColumn]))
            cursorColumn++;

        while (cursorColumn < lines[cursorLine].length && isWordChar(lines[cursorLine].buffer[cursorColumn]))
            cursorColumn++;

        memmove(&lines[cursorLine].buffer[oldColumn],&lines[cursorLine].buffer[cursorColumn],lines[cursorLine].length - cursorColumn + 1);

        lines[cursorLine].length -= cursorColumn - oldColumn;
        cursorColumn = oldColumn;
        contentWidthDirty = 1;
        if (cursorColumn == lines[cursorLine].length && cursorLine < lineCount - 1) {
            int currentLength = lines[cursorLine].length;
            int nextLength = lines[cursorLine + 1].length;
            if (!growLine(&lines[cursorLine], currentLength + nextLength + 1)) {
                undo_push(2); // OOM: merge refused
                return;
            }
            memcpy(&lines[cursorLine].buffer[currentLength],lines[cursorLine + 1].buffer,nextLength + 1);
            lines[cursorLine].length += nextLength;
            contentWidthDirty = 1;
            free(lines[cursorLine + 1].buffer);
            memmove(&lines[cursorLine + 1],&lines[cursorLine + 2],(lineCount - cursorLine - 2) * sizeof(Line));
            lineCount--;
        }
        undo_push(1);
    }
    if (keyRepeat(KEY_DELETE) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn < lines[cursorLine].length) {
            undo_run_line(); // coalesce char deletes on a line
            memmove(&lines[cursorLine].buffer[cursorColumn],&lines[cursorLine].buffer[cursorColumn + 1],lines[cursorLine].length - cursorColumn);

            lines[cursorLine].length--;
            contentWidthDirty = 1;
        }
        else if (cursorLine < lineCount - 1) {
            undo_discrete(cursorLine, 2); // close the run; merge is discrete
            int currentLength = lines[cursorLine].length;
            int nextLength = lines[cursorLine + 1].length;
            if (!growLine(&lines[cursorLine], currentLength + nextLength + 1)) {
                undo_push(2); // OOM: merge refused
                return;
            }
            memcpy(&lines[cursorLine].buffer[currentLength],lines[cursorLine + 1].buffer,nextLength + 1);
            lines[cursorLine].length += nextLength;
            contentWidthDirty = 1;
            free(lines[cursorLine + 1].buffer);
            memmove(&lines[cursorLine + 1],&lines[cursorLine + 2],(lineCount - cursorLine - 2) * sizeof(Line));
            lineCount--;
            undo_push(1);
        }
    }
}

void textstuff(Font usedfont, float fontsize) {
    selection();

    // Any mouse press/release finalizes a coalescing run (the caret may
    // jump, or this may start a drag-select to fold).
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        undo_push(1);

    // Monospace advance per column. Spacing 0.8 adds per char, so measure a
    // 10-char run and divide (single-glyph measure undercounts the advance).
    charW = MeasureTextEx(usedfont, "MMMMMMMMMM", fontsize, 0.8).x / 10.0f;

    // Visible line range: everything outside [first,last) is skipped, not
    // merely scissored — the CPU no longer measures/draws offscreen lines.
    int first = scrollLine;
    if (first < 0) first = 0;
    int last = scrollLine + (int)(ScreenRect.height / lineHeight) + 2;
    if (last > lineCount) last = lineCount;

    // contentWidth only rescans on edit frames or zoom changes (charW moves):
    // strlen sweep, no glyph work.
    static float lastCharW = 0;
    if (contentWidthDirty || charW != lastCharW) {
        cachedContentWidth = 0;
        for (int i = 0; i < lineCount; i++) {
            float width = strlen(lines[i].buffer) * charW;
            if (width > cachedContentWidth)
                cachedContentWidth = width;
        }
        contentWidthDirty = 0;
        lastCharW = charW;
    }
    horscroll=scrollbarHorizontal(startX,ScreenRect.y + ScreenRect.height,ScreenRect.width-5,cachedContentWidth,&scrollX);


    BeginScissorMode(ScreenRect.x, ScreenRect.y+20, ScreenRect.width-3, ScreenRect.height-20);

    navigation();

    scrollMouse(lineCount, ScreenRect.height, lineHeight, &scrollLine,
                ScreenRect.width - 5, cachedContentWidth, &scrollX);

    mouseSelection(usedfont, fontsize);

    drawSelection(usedfont, fontsize);
    for (int i = first; i < last; i++) {
        DrawTextEx(usedfont,lines[i].buffer,(Vector2){startX-scrollX,startY + (i - scrollLine) * lineHeight},fontsize,0.8,WHITE);

    }


    int keypressed = GetCharPressed();
    while (keypressed > 0) {
        if (selecting) {
            undo_run_selection(); // typing over a selection folds into a run
            deleteSelection();
        } else {
            undo_run_line();      // coalesce typed chars on the same line
        }
        Line *line = &lines[cursorLine];

        if (line->length + 1 >= line->capacity &&
            !growLine(line, line->capacity * 2)) {
            oomRefuse();
            break; // OOM: character refused, buffer untouched
        }
        memmove(&line->buffer[cursorColumn + 1],&line->buffer[cursorColumn],line->length - cursorColumn + 1);
        line->buffer[cursorColumn] = (char)keypressed;
        line->length++;
        contentWidthDirty = 1;
        cursorColumn++;
        line->buffer[line->length] = '\0';
        keypressed = GetCharPressed();
    }
    // Hello World
    if (IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)) {
        undo_discrete(cursorLine, 1);
        int remainingLength = lines[cursorLine].length - cursorColumn;
        if (!addLine(cursorLine + 1)) {
            oomRefuse(); // OOM: cannot split, Enter refused
            undo_drop();
        } else {
            if (!growLine(&lines[cursorLine + 1], remainingLength + 1)) {
                // OOM: the new line stays empty; degrade to plain newline.
                oomRefuse();
            } else {
                memmove(lines[cursorLine + 1].buffer,&lines[cursorLine].buffer[cursorColumn],remainingLength + 1);
                lines[cursorLine + 1].length = remainingLength;
                lines[cursorLine].buffer[cursorColumn] = '\0';
                lines[cursorLine].length = cursorColumn;
            }
            contentWidthDirty = 1;
            cursorLine++;
            cursorColumn = 0;
            undo_push(2);
        }
    }
    if (IsKeyPressed(KEY_TAB)) {
        undo_discrete(cursorLine, 1);
        Line *line = &lines[cursorLine];
        if ((line->length + 1 >= line->capacity && !growLine(line, line->capacity * 2))) {
            oomRefuse(); // OOM: Tab refused, buffer untouched
            undo_drop();
        } else {
            memmove(&line->buffer[cursorColumn + 1],&line->buffer[cursorColumn],line->length - cursorColumn + 1);
            line->buffer[cursorColumn] = '\t';
            line->length++;
            contentWidthDirty = 1;
            cursorColumn++;
            line->buffer[line->length] = '\0';
            undo_push(1);
        }
    }
    float cursorWidth = cursorColumn * charW;
    if (GetKeyPressed() != 0) {
    scrollHorizontal(cursorWidth, ScreenRect.width, &scrollX);
        // some key was pressed
    }
    cursor(usedfont, fontsize, startX, startY, lineHeight);
    EndScissorMode();
    vertscroll=scrollbar(ScreenRect.y,ScreenRect.height,lineCount,lineHeight,&scrollLine);
    //terminal_height=make_terminal(ScreenRect.height);

    // Error feedback: flash above the bottom ribbon whenever an operation
    // was refused or failed (OOM, save failure, ...).
    if (statusUntil > 0.0 && GetTime() < statusUntil) {
        DrawTextEx(usedfont, statusMsg,
                   (Vector2){10, GetScreenHeight() - fontsize * 1.05f - 24},
                   fontsize * 0.8f, 0, RED);
    }

    // OOM drill (debug): Ctrl+Alt+O arms a countdown; the 5th grow attempt
    // then fails as if out of memory. Verify the editor survives the drill.
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_O)) {
        oomDrillCountdown = 5;
    }
}


//Todo shift+home, shift+end
