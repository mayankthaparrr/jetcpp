//
// Created by Mayank Thapar on 10-09-2026.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "ui_textarea.h"
#include "ui_scroll.h"
#include "ui_ribbon.h"
#include "ui_ribbonEvents.h"
#include "ui_messages.h"
#include "../func/undo.h"
#include "../func/lsp.h"
#include "ui_syntax.h"

//inits vars
int scrollLine = 0;
float scrollX = 0;
Rectangle ScreenRect;
Vector2 fname_size;

// Render-clip state (refreshed every frame in borders()):
// char_width = monospace advance per column, used instead of MeasureTextEx everywhere.
// cachedContentWidth feeds the horizontal scrollbar; only rescanned on edits.
static float char_width = 10.0f;
static float cachedContentWidth = 0;
int contentWidthDirty = 1;

#define TEXT_SPACING 0.8f // DrawTextEx spacing: added after EVERY glyph
#define TAB_WIDTH 4       // tab stops every 4 columns

// ---------------------------------------------------------------------------
// Byte offsets vs screen columns. Line buffers hold UTF-8 bytes, but the
// screen has one column per codepoint and tabs jump to the next tab stop.
// Codepoints are decoded with raylib's own GetCodepointNext, so this math
// always agrees with what DrawTextEx draws (bad bytes = one '?' each).
// Cursor/selection columns stay BYTE offsets, always on codepoint boundaries.

// Bytes in the codepoint starting at text (0 at the terminator).
static int codepointSize(const char *text) {
    if (*text == '\0') return 0;
    if ((unsigned char)*text < 0x80) return 1;
    int n = 1;
    GetCodepointNext(text, &n);
    return n;
}

// Screen column where byte offset byteCol of text starts.
static int visualCol(const char *text, int byteCol) {
    int col = 0;
    for (int i = 0; i < byteCol && text[i];) {
        if (text[i] == '\t') col = (col / TAB_WIDTH + 1) * TAB_WIDTH;
        else col++;
        i += codepointSize(&text[i]);
    }
    return col;
}

// Byte offset of the last codepoint that starts at or before screen column
// target (clamps to the end of the line). Used to keep the column on Up/Down.
static int byteFromVisual(const char *text, int target) {
    int col = 0, i = 0;
    while (text[i]) {
        int next = text[i] == '\t' ? (col / TAB_WIDTH + 1) * TAB_WIDTH : col + 1;
        if (next > target) break;
        col = next;
        i += codepointSize(&text[i]);
    }
    return i;
}

// Byte offset nearest to pixel x (relative to the line's first column):
// clicking the left half of a glyph puts the caret before it.
static int byteFromX(const char *text, float x) {
    int col = 0, i = 0;
    while (text[i]) {
        int next = text[i] == '\t' ? (col / TAB_WIDTH + 1) * TAB_WIDTH : col + 1;
        if (x < (col + next) * 0.5f * char_width) break;
        col = next;
        i += codepointSize(&text[i]);
    }
    return i;
}

// Start of the codepoint before byteCol (0 if byteCol is 0).
static int prevBoundary(const char *text, int byteCol) {
    int i = 0, prev = 0;
    while (i < byteCol && text[i]) {
        prev = i;
        i += codepointSize(&text[i]);
    }
    return prev;
}
// ---------------------------------------------------------------------------

float startX ;
float startY ;
float lineHeight ;
int cursorLine = 0;
int cursorColumn = 0;
Line *lines=NULL;
int lineCount=0;
int selectionLine = 0;
int selectionColumn = 0;
int selecting = 0;
float zoom = 1.0f; // font size multiplier (Ctrl+wheel / Ctrl+'+')
float ribbon_height=5;

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
Rectangle terminalRect;

// for each line: does it start inside a /* */ comment. redone on every edit
static unsigned char *lineInComment = NULL;
static int lineInCommentCap = 0;
static int lineCommentCount = 0; // lines filled in on the last redo

// Per-frame text metrics, measured BEFORE the layout so initrect() can size
// the scrollbars from this frame's content.
static void measureTextMetrics(Font usedfont, float fontsize) {
    // Exact per-column advance: glyph advance + the spacing DrawTextEx adds
    // after every glyph. (MeasureTextEx counts only n-1 spacings for n glyphs,
    // so dividing a measured 10-char run drifted ~0.08px per column.)
    char_width = MeasureTextEx(usedfont, "J", fontsize, 0).x + TEXT_SPACING;
    lineHeight = MeasureTextEx(usedfont, "Jet", fontsize, 0).y;

    // contentWidth only rescans on edit frames or zoom changes (char_width moves):
    // byte sweep, no glyph work.
    static float lastchar_width = 0;
    if (contentWidthDirty || char_width != lastchar_width) {
        if (contentWidthDirty) lspChanged(); // the text changed: clangd gets it once typing pauses
        if (lineCount > lineInCommentCap) {
            unsigned char *grown = realloc(lineInComment, lineCount * 2);
            if (grown) {
                lineInComment = grown;
                lineInCommentCap = lineCount * 2;
            }
        }
        cachedContentWidth = 0;
        int inComment = 0;
        for (int i = 0; i < lineCount; i++) {
            float width = visualCol(lines[i].buffer, lines[i].length) * char_width;
            if (width > cachedContentWidth)
                cachedContentWidth = width;
            if (i < lineInCommentCap) lineInComment[i] = inComment; // out of memory: comments just won't span lines
            inComment = syntaxLine(lines[i].buffer, lines[i].length, inComment, NULL);
        }
        lineCommentCount = lineCount < lineInCommentCap ? lineCount : lineInCommentCap;
        contentWidthDirty = 0;
        lastchar_width = char_width;
    }
}

void initrect() {
    ScreenRect=(Rectangle){GetScreenWidth()*0.005,ribbon_height*1.4,GetScreenWidth()*0.99,GetScreenHeight()-ribbon_height*2.8};
    // the Message pane takes the bottom of the editor while it's open
    terminal_height = messagesOpen() ? make_terminal(ScreenRect.height) : 0;
    ScreenRect.height-=terminal_height;
    terminalRect=(Rectangle){ScreenRect.x,ScreenRect.y+ScreenRect.height,ScreenRect.width,terminal_height};

    // K7: decide scrollbar visibility from THIS frame's content (it used to
    // reuse last frame's scrollbar() results). Tested on the inner rect that
    // borders() hands the scrollbars; a horizontal bar steals height, which
    // can in turn make the vertical one necessary.
    float innerW = ScreenRect.width - 10;
    float innerH = ScreenRect.height - 10;
    int needV = scrollbarNeeded(innerH, lineCount, lineHeight);
    int needH = scrollbarHorizontalNeeded(innerW - (needV ? 15 : 0) - 5, cachedContentWidth);
    if (needH)
        needV = scrollbarNeeded(innerH - 10, lineCount, lineHeight);
    vertscroll = needV ? 15 : 0;
    horscroll = needH ? 10 : 0;

    ScreenRect.width-=vertscroll;
    ScreenRect.height-=horscroll;
}

void borders(float fontsize,char *filename,Font usedfont) {
    ribbon_height=fontsize*1.05;
    measureTextMetrics(usedfont, fontsize);
    initrect();
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
    float cursorX = startX -scrollX + visualCol(lines[cursorLine].buffer, cursorColumn) * char_width;
    float cursorY = startY + (cursorLine - scrollLine) * lineHeight;
    DrawTextEx(GetFontDefault(), "|", (Vector2){cursorX-3, cursorY}, fontsize , 0, RED);
}

// N3/K3: keep the caret in view whenever it moved since the last sync —
// keys, edits, Enter, paste, undo, menu actions. Wheel and scrollbar
// scrolling never move the caret, so this never fights them.
static int followLine = -1;
static int followColumn = -1;

static void followCaret(void) {
    if (cursorLine == followLine && cursorColumn == followColumn)
        return;
    scrollKeyboard(cursorLine, lineHeight, ScreenRect.height-40, &scrollLine);
    scrollHorizontal(visualCol(lines[cursorLine].buffer, cursorColumn) * char_width,
                     ScreenRect.width, &scrollX);
    followLine = cursorLine;
    followColumn = cursorColumn;
}

// The caret moved but the view should stay put: mouse clicks (the caret is
// already where the user is looking — chasing it mid-drag would autoscroll)
// and Select All.
void caretKeepView(void) {
    followLine = cursorLine;
    followColumn = cursorColumn;
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
    int shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    int moveKey = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
                  IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
                  IsKeyPressed(KEY_HOME) || IsKeyPressed(KEY_END);

    if (!selecting && shift && moveKey) {
        selectionLine = cursorLine;
        selectionColumn = cursorColumn;
        selecting = 1;
    }

    // A plain (unshifted) Up/Down/Home/End ends the selection instead of
    // dragging it along. Left/Right are handled in navigation(): they
    // collapse to the selection edge.
    if (selecting && !shift &&
        (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
         IsKeyPressed(KEY_HOME) || IsKeyPressed(KEY_END))) {
        selecting = 0;
    }

    if (!IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) &&
        !IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        selectionLine == cursorLine && selectionColumn == cursorColumn) {
        selecting = 0;
    }
}
void posFromMouse(Vector2 mouse, int *line, int *col) {
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

    // Same column math as drawing (tabs, UTF-8): no per-click glyph measuring.
    *col = byteFromX(lines[*line].buffer, mouse.x - startX + scrollX);
}

void mouseSelection(void) {
    static int dragging = 0;
    static int dragMoved = 0;

    // An open menu owns the mouse: a click on (or outside) its dropdown must
    // not also place the caret / start a drag in the text underneath.
    if (ribbonMenuOpen())
        return;

    Vector2 mouse = GetMousePosition();

    if (mouse.x < ScreenRect.x || mouse.x > ScreenRect.x + ScreenRect.width ||
        mouse.y < ScreenRect.y || mouse.y > ScreenRect.y + ScreenRect.height)
        return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int line;
        int column;

        posFromMouse(mouse, &line, &column);

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

        posFromMouse(mouse, &line, &column);

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

void drawSelection(void) {
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

        // Monospace: screen column * advance, no measuring, no VLAs.
        int startCol = visualCol(lines[i].buffer, lineStart);
        float x = startX -scrollX + startCol * char_width;
        float width = (visualCol(lines[i].buffer, lineEnd) - startCol) * char_width;

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

// Returns 1 if anything was inserted (0: empty clipboard or OOM refusal).
int pasteClipboard(void) {
    const char *clipboard = GetClipboardText();

    if (clipboard == NULL || clipboard[0] == '\0')
        return 0;

    int pasteLength = strlen(clipboard);

    if (strchr(clipboard, '\n') == NULL) {
        Line *line = &lines[cursorLine];

        if (line->length + pasteLength + 1 > line->capacity &&
            !growLine(line, line->length + pasteLength + 1)) {
            return 0; // OOM: paste refused, buffer untouched
        }

        memmove(&line->buffer[cursorColumn + pasteLength],
                &line->buffer[cursorColumn],
                line->length - cursorColumn + 1);

        memcpy(&line->buffer[cursorColumn], clipboard, pasteLength);

        line->length += pasteLength;
        contentWidthDirty = 1;
        cursorColumn += pasteLength;
        return 1;
    }

    // Multi-line: build every new line in full BEFORE touching the document,
    // so OOM refuses the whole paste instead of leaving a NULL line behind.
    int beforeLength = cursorColumn;
    int afterLength = lines[cursorLine].length - cursorColumn;
    const char *current = lines[cursorLine].buffer;

    int newLines = 1;

    for (int i = 0; clipboard[i] != '\0'; i++) {
        if (clipboard[i] == '\n')
            newLines++;
    }

    char **bufs = calloc(newLines, sizeof(char *));
    if (!bufs) {
        showStatusError("OUT OF MEMORY - paste refused");
        return 0;
    }

    const char *start = clipboard;

    for (int i = 0; i < newLines; i++) {
        const char *newline = strchr(start, '\n');
        int length = newline ? (int)(newline - start) : (int)strlen(start);

        // Strip a trailing carriage return (Windows CRLF clipboard text)
        // so it doesn't end up inside the line buffer.
        while (length > 0 && start[length - 1] == '\r')
            length--;

        int extra = (i == 0 ? beforeLength : 0) +
                    (i == newLines - 1 ? afterLength : 0);

        bufs[i] = malloc(length + extra + 1);
        if (!bufs[i]) {
            for (int j = 0; j < i; j++) free(bufs[j]);
            free(bufs);
            showStatusError("OUT OF MEMORY - paste refused");
            return 0;
        }

        int position = 0;

        if (i == 0) {
            memcpy(bufs[i], current, beforeLength);
            position += beforeLength;
        }

        memcpy(&bufs[i][position], start, length);
        position += length;

        if (i == newLines - 1) {
            memcpy(&bufs[i][position], &current[cursorColumn], afterLength);
            position += afterLength;
        }

        bufs[i][position] = '\0';

        if (newline != NULL)
            start = newline + 1;
    }

    int oldLineCount = lineCount;

    if (!growLines(lineCount + newLines - 1)) {
        for (int i = 0; i < newLines; i++) free(bufs[i]); // OOM: refuse the multi-line paste entirely
        free(bufs);
        return 0;
    }

    memmove(&lines[cursorLine + newLines],
            &lines[cursorLine + 1],
            (oldLineCount - cursorLine - 1) * sizeof(Line));

    // The slot at cursorLine still holds the old line's buffer; its text is
    // already copied into bufs[0] / bufs[last], so free it before replacing.
    free(lines[cursorLine].buffer);

    lineCount += newLines - 1;

    for (int i = 0; i < newLines; i++) {
        Line *l = &lines[cursorLine + i];
        l->buffer = bufs[i];
        l->length = (int)strlen(bufs[i]);
        l->capacity = l->length + 1;
    }
    free(bufs);

    contentWidthDirty = 1;
    cursorLine += newLines - 1;
    cursorColumn = lines[cursorLine].length - afterLength;
    return 1;
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
        selectall(); // same as Edit -> Select All (keeps the view put)
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
        int hadSelection = selecting;
        if (selecting) {
            undo_run_selection(); // one step: paste swallows the selection
            deleteSelection();
        } else {
            undo_discrete(cursorLine, 1);
        }
        int beforeLineCount = lineCount;
        if (pasteClipboard() || hadSelection)
            undo_push(lineCount - beforeLineCount + 1);
        else
            undo_drop(); // empty clipboard / OOM: nothing changed, no empty undo step
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

    // Word jumps stop at line edges: only a jump that STARTS at column 0 /
    // end of line crosses to the neighbouring line.
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_LEFT)) {
        if (cursorColumn == 0) {
            if (cursorLine > 0) {
                cursorLine--;
                cursorColumn = lines[cursorLine].length;
            }
        } else {
            while (cursorColumn > 0 && !isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
                cursorColumn--;

            while (cursorColumn > 0 && isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
                cursorColumn--;
        }
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_RIGHT)) {
        if (cursorColumn == lines[cursorLine].length) {
            if (cursorLine < lineCount - 1) {
                cursorLine++;
                cursorColumn = 0;
            }
        } else {
            while (cursorColumn < lines[cursorLine].length && !isWordChar(lines[cursorLine].buffer[cursorColumn]))
                cursorColumn++;

            while (cursorColumn < lines[cursorLine].length && isWordChar(lines[cursorLine].buffer[cursorColumn]))
                cursorColumn++;
        }
        scrollKeyboard(cursorLine, lineHeight, ScreenRect.height, &scrollLine);
    }

    if (keyRepeat(KEY_LEFT) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn > 0) {
            cursorColumn = prevBoundary(lines[cursorLine].buffer, cursorColumn); // whole UTF-8 char
        }
        else if (cursorLine > 0) {
            cursorLine--;
            cursorColumn = lines[cursorLine].length;
        }
    }

    if (keyRepeat(KEY_RIGHT) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn < lines[cursorLine].length) {
            cursorColumn += codepointSize(&lines[cursorLine].buffer[cursorColumn]); // whole UTF-8 char
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

    // Up/Down keep the SCREEN column (tabs and UTF-8 make it differ from the
    // byte column), clamped to the end of shorter lines.
    if (keyRepeat(KEY_UP) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL) && cursorLine > 0) {
        int col = visualCol(lines[cursorLine].buffer, cursorColumn);
        cursorLine--;
        cursorColumn = byteFromVisual(lines[cursorLine].buffer, col);

            scrollKeyboard(cursorLine, lineHeight, ScreenRect.height, &scrollLine);
    }

    if (keyRepeat(KEY_DOWN) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL) && cursorLine < lineCount - 1) {
        int col = visualCol(lines[cursorLine].buffer, cursorColumn);
        cursorLine++;
        cursorColumn = byteFromVisual(lines[cursorLine].buffer, col);

        scrollKeyboard(cursorLine, lineHeight, ScreenRect.height-40, &scrollLine);
    }

    // Home/End: start/end of the line. Ctrl+Home/Ctrl+End: whole document.
    if (IsKeyPressed(KEY_HOME)) {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
            cursorLine = 0;
        cursorColumn = 0;
        scrollKeyboard(cursorLine, lineHeight, ScreenRect.height-40, &scrollLine);
    }



    if (IsKeyPressed(KEY_END)) {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
            cursorLine = lineCount - 1;
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

    // Ctrl+Backspace deletes back to the word start and stops at column 0;
    // only a press AT column 0 joins the line onto the previous one.
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_BACKSPACE)) {
        if (cursorColumn == 0) {
            if (cursorLine > 0) {
                undo_discrete(cursorLine - 1, 2);
                int previousLength = lines[cursorLine - 1].length;
                int currentLength = lines[cursorLine].length;

                if (!growLine(&lines[cursorLine - 1], previousLength + currentLength + 1)) {
                    undo_push(2); // OOM: merge refused, nothing deleted
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
                undo_push(1);
            }
        } else {
            undo_discrete(cursorLine, 1);

            int oldColumn = cursorColumn;
            while (cursorColumn > 0 && !isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
                cursorColumn--;

            while (cursorColumn > 0 && isWordChar(lines[cursorLine].buffer[cursorColumn - 1]))
                cursorColumn--;

            memmove(&lines[cursorLine].buffer[cursorColumn],&lines[cursorLine].buffer[oldColumn],lines[cursorLine].length - oldColumn + 1);
            lines[cursorLine].length -= oldColumn - cursorColumn;
            contentWidthDirty = 1;
            undo_push(1);
        }
    }


    if (keyRepeat(KEY_BACKSPACE) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn > 0) {
            undo_run_line(); // coalesce char backspaces on a line
            int from = prevBoundary(lines[cursorLine].buffer, cursorColumn); // whole UTF-8 char
            memmove(&lines[cursorLine].buffer[from],&lines[cursorLine].buffer[cursorColumn],lines[cursorLine].length - cursorColumn + 1);

            lines[cursorLine].length -= cursorColumn - from;
            cursorColumn = from;
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

    // Ctrl+Delete deletes to the word end and stops at the end of the line;
    // only a press AT the end of the line joins the next line on.
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_DELETE)) {
        if (cursorColumn == lines[cursorLine].length) {
            if (cursorLine < lineCount - 1) {
                undo_discrete(cursorLine, 2);
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
        } else {
            undo_discrete(cursorLine, 1);

            int oldColumn = cursorColumn;
            while (cursorColumn < lines[cursorLine].length && !isWordChar(lines[cursorLine].buffer[cursorColumn]))
                cursorColumn++;

            while (cursorColumn < lines[cursorLine].length && isWordChar(lines[cursorLine].buffer[cursorColumn]))
                cursorColumn++;

            memmove(&lines[cursorLine].buffer[oldColumn],&lines[cursorLine].buffer[cursorColumn],lines[cursorLine].length - cursorColumn + 1);

            lines[cursorLine].length -= cursorColumn - oldColumn;
            cursorColumn = oldColumn;
            contentWidthDirty = 1;
            undo_push(1);
        }
    }
    if (keyRepeat(KEY_DELETE) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (cursorColumn < lines[cursorLine].length) {
            undo_run_line(); // coalesce char deletes on a line
            int n = codepointSize(&lines[cursorLine].buffer[cursorColumn]); // whole UTF-8 char
            memmove(&lines[cursorLine].buffer[cursorColumn],&lines[cursorLine].buffer[cursorColumn + n],lines[cursorLine].length - cursorColumn - n + 1);

            lines[cursorLine].length -= n;
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

// what each byte of the line being drawn is (SYN_*)
static unsigned char *lineKinds = NULL;
static int lineKindsCap = 0;

// colours line n: the scanner first, then clangd's types/functions/macros on
// top (only over plain text, never inside comments or strings)
static int colourLine(int n) {
    Line *l = &lines[n];
    if (l->length + 1 > lineKindsCap) {
        unsigned char *grown = realloc(lineKinds, l->length + 1);
        if (!grown) return 0; // out of memory: draw it white
        lineKinds = grown;
        lineKindsCap = l->length + 1;
    }
    int inComment = n < lineCommentCount ? lineInComment[n] : 0;
    syntaxLine(l->buffer, l->length, inComment, lineKinds);

    int count;
    const LspToken *t = lspTokens(&count);
    // tokens are in file order, so find this line's first one by halving
    int lo = 0, hi = count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (t[mid].line < n) lo = mid + 1;
        else hi = mid;
    }
    for (int k = lo; k < count && t[k].line == n; k++) {
        if (t[k].col < 0 || t[k].col + t[k].len > l->length) continue; // from before an edit
        int kind = syntaxKindFromLsp(lspTypeName(t[k].type));
        if (kind == SYN_TEXT) continue;
        for (int b = t[k].col; b < t[k].col + t[k].len; b++) {
            if (lineKinds[b] == SYN_TEXT) lineKinds[b] = kind;
        }
    }
    return 1;
}

// Draw one line in runs. A run ends at a tab (the next run starts at the next
// tab stop, DrawTextEx alone would give a tab one column) or where the colour changes.
static void drawLineText(Font usedfont, float fontsize, int n, Vector2 pos) {
    char *text = lines[n].buffer;
    int coloured = syntaxOn() && colourLine(n);
    if (!coloured && !strchr(text, '\t')) {
        DrawTextEx(usedfont, text, pos, fontsize, TEXT_SPACING, WHITE);
        return;
    }
    int col = 0, i = 0;
    while (text[i]) {
        if (text[i] == '\t') {
            col = (col / TAB_WIDTH + 1) * TAB_WIDTH;
            i++;
            continue;
        }
        int runStart = i, runCol = col;
        int kind = coloured ? lineKinds[i] : SYN_TEXT;
        while (text[i] && text[i] != '\t' && (!coloured || lineKinds[i] == kind)) {
            i += codepointSize(&text[i]);
            col++;
        }
        char saved = text[i];
        text[i] = '\0'; // end the run in place; restored right after
        DrawTextEx(usedfont, &text[runStart], (Vector2){pos.x + runCol * char_width, pos.y},
                   fontsize, TEXT_SPACING, coloured ? syntaxColor(kind) : WHITE);
        text[i] = saved;
    }
}

void textstuff(Font usedfont, float fontsize) {
    selection();

    // Any mouse press/release finalizes a coalescing run (the caret may
    // jump, or this may start a drag-select to fold).
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        undo_push(1);

    // char_width, lineHeight and cachedContentWidth were measured in borders(),
    // before the layout (K7).
    scrollbarHorizontal(startX,ScreenRect.y + ScreenRect.height,ScreenRect.width-5,cachedContentWidth,&scrollX,horscroll > 0);


    BeginScissorMode(ScreenRect.x, ScreenRect.y+20, ScreenRect.width-3, ScreenRect.height-20);

    navigation();
    followCaret(); // keys, edits and menu actions since the last frame

    if (!messagesHovered()) // the wheel scrolls the Message pane when it's under the mouse
        scrollMouse(lineCount, ScreenRect.height, lineHeight, &scrollLine,
                    ScreenRect.width - 5, cachedContentWidth, &scrollX);

    mouseSelection();
    caretKeepView(); // a mouse-placed caret is already in view: don't chase it

    // Visible line range: everything outside [first,last) is skipped, not
    // merely scissored — the CPU no longer measures/draws offscreen lines.
    // Taken after this frame's scrolling so no edge line goes missing.
    int first = scrollLine;
    if (first < 0) first = 0;
    int last = scrollLine + (int)(ScreenRect.height / lineHeight) + 2;
    if (last > lineCount) last = lineCount;

    drawSelection();
    for (int i = first; i < last; i++) {
        drawLineText(usedfont, fontsize, i,
                     (Vector2){startX-scrollX,startY + (i - scrollLine) * lineHeight});
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

        // Store the typed codepoint as UTF-8 (casting to char mangled é, €...).
        int n = 0;
        const char *utf8 = CodepointToUTF8(keypressed, &n);
        int need = line->length + n + 1;
        if (need > line->capacity &&
            !growLine(line, need > line->capacity * 2 ? need : line->capacity * 2)) {
            oomRefuse();
            break; // OOM: character refused, buffer untouched
        }
        memmove(&line->buffer[cursorColumn + n],&line->buffer[cursorColumn],line->length - cursorColumn + 1);
        memcpy(&line->buffer[cursorColumn], utf8, n);
        line->length += n;
        contentWidthDirty = 1;
        cursorColumn += n;
        line->buffer[line->length] = '\0';
        keypressed = GetCharPressed();
    }
    // Hello World
    if (IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)) {
        int ok = 1;
        int hadSelection = selecting;
        if (selecting) {
            undo_run_selection(); // one step: Enter replaces the selection
            deleteSelection();
            if (selecting) {      // OOM: delete refused, so Enter is too
                undo_drop();
                ok = 0;
            }
        } else {
            undo_discrete(cursorLine, 1);
        }
        int remainingLength = lines[cursorLine].length - cursorColumn;
        if (!ok) {
            // refused above
        } else if (!addLine(cursorLine + 1)) {
            oomRefuse(); // OOM: cannot split, Enter refused
            if (hadSelection) undo_push(1); // the selection delete still happened
            else undo_drop();
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
        int ok = 1;
        int hadSelection = selecting;
        if (selecting) {
            undo_run_selection(); // Tab replaces the selection like a typed char
            deleteSelection();
            if (selecting) {      // OOM: delete refused, so Tab is too
                undo_drop();
                ok = 0;
            }
        } else {
            undo_discrete(cursorLine, 1);
        }
        Line *line = &lines[cursorLine];
        if (!ok) {
            // refused above
        } else if ((line->length + 1 >= line->capacity && !growLine(line, line->capacity * 2))) {
            oomRefuse(); // OOM: Tab refused, buffer untouched
            if (hadSelection) undo_push(1); // the selection delete still happened
            else undo_drop();
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
    cursor(usedfont, fontsize, startX, startY, lineHeight);
    EndScissorMode();
    scrollbar(ScreenRect.y,ScreenRect.height,lineCount,lineHeight,&scrollLine,vertscroll > 0);

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
