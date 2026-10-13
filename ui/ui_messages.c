//this is ui_messages.c
//the Message pane under the editor: compiler output, click an error to jump to it
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "raylib.h"
#include "ui_messages.h"
#include "ui_textarea.h"
#include "ui_ribbon.h"
#include "ui_ribbonEvents.h"

static char **msgs = NULL;
static int msgCount = 0;
static char partial[1024]; // output line that hasn't got its '\n' yet
static int partialLen = 0;
static int msgScroll = 0;
static int paneOpen = 0;

static void pushLine(const char *text) {
    char **grown = realloc(msgs, (msgCount + 1) * sizeof(char *));
    if (!grown) return; // out of memory: drop the line
    msgs = grown;
    size_t n = strlen(text) + 1;
    char *copy = malloc(n);
    if (!copy) return;
    memcpy(copy, text, n);
    msgs[msgCount++] = copy;
}

void messagesClear(void) {
    for (int i = 0; i < msgCount; i++) free(msgs[i]);
    free(msgs);
    msgs = NULL;
    msgCount = 0;
    partialLen = 0;
    msgScroll = 0;
}

void messagesLine(const char *text) {
    if (partialLen > 0) { // finish off output that didn't end in a newline
        partial[partialLen] = '\0';
        pushLine(partial);
        partialLen = 0;
    }
    pushLine(text);
}

void messagesAppend(const char *text, int len) {
    for (int i = 0; i < len; i++) {
        if (text[i] == '\r') continue;
        if (text[i] == '\n') {
            partial[partialLen] = '\0';
            pushLine(partial);
            partialLen = 0;
        } else if (partialLen < (int)sizeof(partial) - 1) {
            partial[partialLen++] = text[i];
        }
    }
}

void messagesShow(int show) {
    paneOpen = show;
}

int messagesOpen(void) {
    return paneOpen;
}

int messagesHovered(void) {
    return paneOpen && CheckCollisionPointRec(GetMousePosition(), terminalRect);
}

// clang errors look like "C:\dir\file.c:12:5: error: ...".
// skip the drive letter first or its ':' looks like the end of the path
static int parseError(const char *s, char *path, int pathSize, int *line, int *col) {
    const char *p = s;
    if (isalpha((unsigned char)p[0]) && p[1] == ':') p += 2;
    const char *colon = strchr(p, ':');
    if (!colon || colon - s >= pathSize) return 0;
    if (sscanf(colon + 1, "%d:%d", line, col) != 2) return 0;
    memcpy(path, s, colon - s);
    path[colon - s] = '\0';
    return 1;
}

static int samePath(const char *a, const char *b) {
    for (; *a && *b; a++, b++) {
        char x = tolower((unsigned char)*a), y = tolower((unsigned char)*b);
        if (x == '/') x = '\\';
        if (y == '/') y = '\\';
        if (x != y) return 0;
    }
    return *a == *b;
}

static void jumpTo(const char *path, int line, int col) {
    const char *current = currentFilepath();
    if (!current || !samePath(path, current)) {
        showStatusError("THAT ERROR IS IN ANOTHER FILE");
        return;
    }
    if (line > lineCount) line = lineCount;
    cursorLine = line > 0 ? line - 1 : 0;
    cursorColumn = col > 0 ? col - 1 : 0; // clang counts from 1, in bytes like cursorColumn
    if (cursorColumn > lines[cursorLine].length) cursorColumn = lines[cursorLine].length;
    selecting = 0;
}

void drawMessages(Font font, float fontsize) {
    if (!paneOpen) return;
    Rectangle r = terminalRect;
    Rectangle in = { r.x + 5, r.y + 5, r.width - 10, r.height - 10 };
    Color tooblue = GetColor(0x0000aaff);
    DrawRectangleLinesEx(r, 2, WHITE);
    DrawRectangleLinesEx(in, 2, WHITE);

    // title sits in the top border, like the file name
    Vector2 title = MeasureTextEx(font, "Message", fontsize, 0);
    DrawRectangle((GetScreenWidth() - title.x) / 2 - 8, r.y, title.x + 16, title.y, tooblue);
    DrawTextEx(font, "Message", (Vector2){ (GetScreenWidth() - title.x) / 2, r.y + 2 }, fontsize, 0, WHITE);

    // [x] closes the pane
    Vector2 xs = MeasureTextEx(font, "[x]", fontsize, 0);
    Rectangle closeBox = { r.x + 15, r.y, xs.x + 8, xs.y };
    DrawRectangleRec(closeBox, tooblue);
    DrawTextEx(font, "[x]", (Vector2){ closeBox.x + 4, r.y + 2 }, fontsize, 0, WHITE);

    Vector2 mouse = GetMousePosition();
    int menu = ribbonMenuOpen(); // an open menu owns the click
    if (!menu && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, closeBox)) {
        paneOpen = 0;
        return;
    }

    float lh = MeasureTextEx(font, "Jet", fontsize, 0).y;
    float top = r.y + title.y + 4;
    float bottom = r.y + r.height - 8;
    int visible = (int)((bottom - top) / lh);
    if (visible < 1) visible = 1;

    if (messagesHovered()) msgScroll -= (int)(GetMouseWheelMove() * 3);
    int maxScroll = msgCount - visible;
    if (maxScroll < 0) maxScroll = 0;
    if (msgScroll > maxScroll) msgScroll = maxScroll;
    if (msgScroll < 0) msgScroll = 0;

    BeginScissorMode(in.x + 2, top, in.width - 4, bottom - top);
    for (int i = 0; i < visible && msgScroll + i < msgCount; i++) {
        const char *s = msgs[msgScroll + i];
        Rectangle row = { in.x + 2, top + i * lh, in.width - 4, lh };
        char path[1024];
        int line, col;
        // error lines light up under the mouse, click to go there
        if (!menu && CheckCollisionPointRec(mouse, row) && parseError(s, path, sizeof(path), &line, &col)) {
            DrawRectangleRec(row, BLUE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) jumpTo(path, line, col);
        }
        DrawTextEx(font, s, (Vector2){ in.x + 10, row.y }, fontsize, 0.8f, WHITE);
    }
    EndScissorMode();
}
