//
// undo.c — linear undo/redo history for the editor buffer.
//
// Each node stores the full contents of the lines an edit touched, both
// before and after, plus the caret to restore. Undo swaps the node's "after"
// region back to its "before" region; redo does the reverse. The redo tail is
// pruned the moment a new edit lands and the list is capped at MAX_UNDO_STEPS
// (newest edits win). History lives purely in RAM and is cleared on load.
//
// Splicing mirrors the pasteClipboard pattern but atomically: every insertion
// buffer is pre-allocated before the document is touched, so a failed apply
// leaves the buffer intact and the node re-undoable.
#include "undo.h"
#include <stdlib.h>
#include <string.h>
#include "ui/ui_textarea.h"
#include "ui/ui_scroll.h"

static UndoNode history[MAX_UNDO_STEPS];
static int historyCount = 0;    // committed nodes (with undo, then redo)
static int historyCursor = 0;   // next node to redo == number of undoable steps
static UndoNode pending;        // in-progress node (coalescing run or discrete)
static int pendingActive = 0;

static char *dupStr(const char *s) {
    int n = (int)strlen(s);
    char *copy = malloc((size_t)n + 1);
    if (!copy) return NULL;
    memcpy(copy, s, (size_t)n + 1);
    return copy;
}

// Snapshot the live buffer region [start, start+count). Owns the result.
// Returns NULL on OOM; any partially captured strings are freed.
static char **captureLines(int start, int count) {
    char **out = calloc((size_t)count, sizeof(char *));
    if (!out) return NULL;
    for (int i = 0; i < count; i++) {
        out[i] = dupStr(lines[start + i].buffer);
        if (!out[i]) {
            for (int j = 0; j < i; j++) free(out[j]);
            free(out);
            return NULL;
        }
    }
    return out;
}

static void freeLineArray(char **arr, int count) {
    if (!arr) return;
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

static void freeNode(UndoNode *n) {
    freeLineArray(n->beforeLns, n->beforeCount);
    freeLineArray(n->afterLns, n->afterCount);
    n->beforeLns = NULL;
    n->afterLns = NULL;
    n->beforeCount = 0;
    n->afterCount = 0;
}

static void oomHistoryClear(void) {
    showStatusError("OUT OF MEMORY - undo history cleared");
    undo_reset();
}

// Replace lines [start, start+removeN) with ins[0..insN). Returns 1 on
// success. All insertion strings are copied before any mutation, and removed
// buffers are freed before any memmove so struct copies created by the shift
// never have their buffers freed twice.
static int replaceLines(int start, int removeN, char **ins, int insN) {
    char **newBufs = malloc((size_t)insN * sizeof(char *));
    if (!newBufs) return 0;
    for (int i = 0; i < insN; i++) {
        size_t n = strlen(ins[i]);
        newBufs[i] = malloc(n + 1);
        if (!newBufs[i]) {
            for (int j = 0; j < i; j++) free(newBufs[j]);
            free(newBufs);
            return 0;
        }
        memcpy(newBufs[i], ins[i], n + 1);
    }

    int oldCount = lineCount;
    int delta = insN - removeN;
    if (oldCount + delta < 1) { // never collapse to an empty document
        for (int i = 0; i < insN; i++) free(newBufs[i]);
        free(newBufs);
        return 0;
    }

    if (delta > 0) {
        Line *grown = realloc(lines, (size_t)(oldCount + delta) * sizeof(Line));
        if (!grown) { // nothing mutated yet — abort cleanly
            for (int i = 0; i < insN; i++) free(newBufs[i]);
            free(newBufs);
            return 0;
        }
        lines = grown;
        memmove(&lines[start + insN], &lines[start + removeN],
                (size_t)(oldCount - start - removeN) * sizeof(Line));
        for (int i = 0; i < removeN; i++) free(lines[start + i].buffer);
    } else {
        // delta <= 0: the region can overlap the shifted tail, so free the
        // removed buffers BEFORE the move to avoid double-freeing aliases.
        for (int i = 0; i < removeN; i++) free(lines[start + i].buffer);
        if (delta < 0) {
            memmove(&lines[start + insN], &lines[start + removeN],
                    (size_t)(oldCount - start - removeN) * sizeof(Line));
            // Shrink best-effort: if realloc fails the surplus tail stays
            // unreachable past lineCount and is never rendered or freed.
            Line *shrunk = realloc(lines, (size_t)(oldCount + delta) * sizeof(Line));
            if (shrunk) lines = shrunk;
        }
    }

    for (int i = 0; i < insN; i++) {
        Line *t = &lines[start + i];
        t->buffer = newBufs[i];
        t->length = (int)strlen(newBufs[i]);
        t->capacity = t->length + 1;
    }
    lineCount = oldCount + delta;

    free(newBufs);
    contentWidthDirty = 1;
    return 1;
}

static void pushPending(void) {
    if (!pendingActive) return;
    if (pending.afterCount < 1 || pending.startLine < 0 ||
        pending.startLine + pending.afterCount > lineCount) {
        undo_drop(); // malformed: discard rather than record garbage
        return;
    }
    pending.afterLns = captureLines(pending.startLine, pending.afterCount);
    if (!pending.afterLns) {
        oomHistoryClear();
        return;
    }
    pending.afterCurL = cursorLine;
    pending.afterCurC = cursorColumn;

    // Any new edit discards the redo branch (VS Code / Zed behavior).
    for (int i = historyCursor; i < historyCount; i++) freeNode(&history[i]);
    historyCount = historyCursor;

    // Cap the stack: evict the oldest node.
    while (historyCount >= MAX_UNDO_STEPS) {
        freeNode(&history[0]);
        for (int i = 1; i < historyCount; i++) history[i - 1] = history[i];
        historyCount--;
        if (historyCursor > 0) historyCursor--;
    }

    history[historyCount] = pending;
    pending.beforeLns = NULL; // ownership moved into the node
    pending.afterLns = NULL;
    pendingActive = 0;
    historyCount++;
    historyCursor = historyCount;
}

// Finalize an open coalescing run. A run never changes lineCount, so its
// region is always the single line it started on.
static void closeRun(void) {
    if (!pendingActive) return;
    pending.afterCount = 1;
    pushPending();
}

// Apply a node onto the live buffer. isUndo=1 swaps after->before.
// Returns 1 on success; OOM leaves the document untouched and the node valid.
static int applyNode(UndoNode *n, int isUndo) {
    int removeN = isUndo ? n->afterCount : n->beforeCount;
    int insN    = isUndo ? n->beforeCount : n->afterCount;
    char **ins  = isUndo ? n->beforeLns : n->afterLns;
    int start   = n->startLine;

    if (start < 0 || start + removeN > lineCount) {
        undo_reset(); // history drifted — drop it rather than walk OOB
        showStatusError("undo history corrupted - reset");
        return 0;
    }
    if (!replaceLines(start, removeN, ins, insN))
        return 0;

    if (isUndo) {
        cursorLine = n->beforeCurL;
        cursorColumn = n->beforeCurC;
    } else {
        cursorLine = n->afterCurL;
        cursorColumn = n->afterCurC;
    }
    if (cursorLine < 0) cursorLine = 0;
    if (cursorLine >= lineCount) cursorLine = lineCount - 1;
    if (cursorColumn < 0) cursorColumn = 0;
    if (cursorColumn > lines[cursorLine].length) cursorColumn = lines[cursorLine].length;

    selecting = 0; // undo/redo never restores a selection
    selectionLine = cursorLine;
    selectionColumn = cursorColumn;
    scrollKeyboard(cursorLine, lineHeight, ScreenRect.height, &scrollLine);
    return 1;
}

void undo_reset(void) {
    for (int i = 0; i < historyCount; i++) freeNode(&history[i]);
    freeLineArray(pending.beforeLns, pending.beforeCount);
    freeLineArray(pending.afterLns, pending.afterCount);
    memset(&pending, 0, sizeof pending);
    historyCount = 0;
    historyCursor = 0;
    pendingActive = 0;
}

void undo_run_line(void) {
    if (pendingActive) {
        if (pending.startLine == cursorLine) return; // same-line run extends
        closeRun();                                   // caret left the line
    }
    if (pendingActive) return;
    char **lns = captureLines(cursorLine, 1);
    if (!lns) { oomHistoryClear(); return; }
    memset(&pending, 0, sizeof pending);
    pending.startLine = cursorLine;
    pending.beforeCount = 1;
    pending.beforeLns = lns;
    pending.beforeCurL = cursorLine;
    pending.beforeCurC = cursorColumn;
    pendingActive = 1;
}

void undo_run_selection(void) {
    closeRun();
    if (pendingActive) return;

    int startLine = selectionLine;
    int startColumn = selectionColumn;
    int endLine = cursorLine;
    int endColumn = cursorColumn;
    if (startLine > endLine || (startLine == endLine && startColumn > endColumn)) {
        int tmpL = startLine, tmpC = startColumn;
        startLine = endLine; startColumn = endColumn;
        endLine = tmpL; endColumn = tmpC;
    }

    char **lns = captureLines(startLine, endLine - startLine + 1);
    if (!lns) { oomHistoryClear(); return; }
    memset(&pending, 0, sizeof pending);
    pending.startLine = startLine;
    pending.beforeCount = endLine - startLine + 1;
    pending.beforeLns = lns;
    pending.beforeCurL = cursorLine;
    pending.beforeCurC = cursorColumn;
    pendingActive = 1;
}

void undo_discrete(int startLine, int beforeCount) {
    closeRun();
    if (pendingActive || startLine < 0 || beforeCount < 1 ||
        startLine + beforeCount > lineCount)
        return;

    char **lns = captureLines(startLine, beforeCount);
    if (!lns) { oomHistoryClear(); return; }
    memset(&pending, 0, sizeof pending);
    pending.startLine = startLine;
    pending.beforeCount = beforeCount;
    pending.beforeLns = lns;
    pending.beforeCurL = cursorLine;
    pending.beforeCurC = cursorColumn;
    pendingActive = 1;
}

void undo_push(int afterCount) {
    if (!pendingActive) return;
    pending.afterCount = afterCount;
    pushPending();
}

void undo_drop(void) {
    if (!pendingActive) return;
    freeLineArray(pending.beforeLns, pending.beforeCount);
    freeLineArray(pending.afterLns, pending.afterCount);
    memset(&pending, 0, sizeof pending);
    pendingActive = 0;
}

void undo_undo(void) {
    closeRun();
    if (historyCursor == 0) return;
    int i = historyCursor - 1;
    if (!applyNode(&history[i], 1)) return;
    historyCursor = i;
}

void undo_redo(void) {
    closeRun();
    if (historyCursor >= historyCount) return;
    int i = historyCursor;
    if (!applyNode(&history[i], 0)) return;
    historyCursor = i + 1;
}