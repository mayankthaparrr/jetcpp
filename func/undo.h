//
// undo.h — linear undo/redo history for the editor buffer (B1 raw-region deltas).
//
// Each history node stores the full contents of the lines an edit touched,
// both before and after, plus the caret positions to restore. Undo swaps a
// node's "after" region back to its "before" region; redo does the reverse.
// Redo history is pruned the moment a new edit lands, and the list is capped
// at MAX_UNDO_STEPS (oldest node evicted first).
// History lives purely in memory; it is cleared by undo_reset() on file load.
#ifndef ATR_UNDO_H
#define ATR_UNDO_H

#define MAX_UNDO_STEPS 100

typedef struct {
    int   startLine;     // region start; valid in both the before and after state
    int   beforeCount;   // lines the region spanned before the edit
    int   afterCount;    // lines the region spanned after the edit
    char **beforeLns;    // beforeCount owned NUL-terminated strings
    char **afterLns;     // afterCount owned NUL-terminated strings
    int   beforeCurL, beforeCurC; // caret restored when undoing
    int   afterCurL,  afterCurC;  // caret restored when redoing
} UndoNode;

// Clear all history (new file / load).
void undo_reset(void);

// Begin or extend a coalescing run on the current single line.
// Typing/backspacing/deleting chars merge into one undo step as long as the
// caret never leaves the line the run opened on. Call BEFORE the mutation.
void undo_run_line(void);

// Fold a selection delete into a typing run: snapshot the whole selected
// multi-line region as the "before" state (it collapses to one line after).
// Call BEFORE deleteSelection() when typing over a selection.
void undo_run_selection(void);

// Begin a discrete one-shot edit over an explicit line region. Closes any
// open coalescing run first. Call BEFORE the mutation.
void undo_discrete(int startLine, int beforeCount);

// Finish the open node: record the live buffer region starting at startLine
// as the "after" state and push it, pruning the redo tail and capping depth.
// afterCount is the number of lines the region spans NOW (1 for line edits,
// 2 for a split, 1 for a merge, ...). Call AFTER the mutation.
void undo_push(int afterCount);

// Discard the open node without recording it (edit was refused / no-op).
void undo_drop(void);

void undo_undo(void);   // Ctrl+Z
void undo_redo(void);   // Ctrl+Y / Ctrl+Shift+Z

#endif