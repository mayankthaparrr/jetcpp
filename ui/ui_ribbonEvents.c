//
// Created by Mayank Thapar on 10-09-2026.
//
#include "ui_ribbonEvents.h"
#include "ui_textarea.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <nfd.h>

char *filename = "NONAME.C";
int quitRequested = 0; // set by File -> Quit; checked in the main loop
static int filenameAllocated = 0;

// Full path of the file backing the current buffer (NULL if never saved).
static char *filepath = NULL;

static void setFilename(const char *newName) {
    if (filenameAllocated) {
        free(filename);
        filenameAllocated = 0;
    }
    size_t len = strlen(newName) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, newName, len);
        filename = copy;
        filenameAllocated = 1;
    }
}

static void setFilepath(const char *newPath) {
    free(filepath);
    filepath = NULL;
    if (newPath) {
        size_t len = strlen(newPath) + 1;
        filepath = malloc(len);
        if (filepath) memcpy(filepath, newPath, len);
    }
}

// Base name of a path, handling both / and \ separators.
static const char *baseNameOf(const char *path) {
    const char *base = strrchr(path, '/');
#ifdef _WIN32
    const char *backslash = strrchr(path, '\\');
    if (backslash && (!base || backslash > base)) base = backslash;
#endif
    return base ? base + 1 : path;
}

// Clear the editor back to a single empty line with the cursor at the top.
static void resetEditor(void) {
    for (int i = 0; i < lineCount; i++) {
        free(lines[i].buffer);
    }

    free(lines);

    lines = NULL;
    lineCount = 0;

    addLine(0);

    cursorLine = 0;
    cursorColumn = 0;

    selectionLine = 0;
    selectionColumn = 0;
    selecting = 0;

    scrollLine = 0;
    scrollColumn = 0;
    scrollX = 0;
}

// Replace a line's contents, growing its buffer if needed.
static void setLineText(int index, const char *text, int len) {
    Line *l = &lines[index];
    if (len + 1 > l->capacity) {
        int newCapacity = l->capacity;
        while (newCapacity < len + 1) newCapacity *= 2;
        char *grown = realloc(l->buffer, newCapacity);
        if (!grown) return;
        l->buffer = grown;
        l->capacity = newCapacity;
    }
    memcpy(l->buffer, text, len);
    l->buffer[len] = '\0';
    l->length = len;
}

void newfile() {
    resetEditor();
    setFilename("NONAME.C");
    setFilepath(NULL);
}

void selectall() {
    selectionLine = 0;
    selectionColumn = 0;
    cursorLine = lineCount - 1;
    cursorColumn = lines[cursorLine].length;
    selecting = 1;
}

// Load `path` into the editor buffer. Used by File -> Open and by
// main() when the program is launched with a file argument.
void loadFileAtPath(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(f);
        return;
    }
    size_t bytesRead = fread(content, 1, size, f);
    fclose(f);
    content[bytesRead] = '\0';

    resetEditor();

    // Split contents on newlines into lines[] (strip carriage returns).
    char *start = content;
    int line = 0;
    for (size_t i = 0; i <= bytesRead; i++) {
        if (content[i] == '\n' || content[i] == '\0') {
            int len = (int)(&content[i] - start);
            while (len > 0 && start[len - 1] == '\r') len--;
            // a trailing newline should not create an extra empty last line
            if (content[i] == '\0' && len == 0 && line > 0) break;
            if (line > 0) addLine(lineCount);
            setLineText(line, start, len);
            line++;
            start = &content[i + 1];
        }
    }
    free(content);

    // Remember where the file lives so Save can write back to it.
    setFilepath(path);
    setFilename(baseNameOf(path));
}

void openfile() {
    nfdchar_t *outPath = NULL;
    if (NFD_OpenDialog(&outPath, NULL, 0, NULL) != NFD_OKAY) {
        return; // cancelled or failed
    }

    loadFileAtPath(outPath);

    NFD_FreePath(outPath);
}

// Write all lines to path, joined with CRLF line endings.
// Returns 1 on success, 0 on failure.
static int writeFile(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    for (int i = 0; i < lineCount; i++) {
        fwrite(lines[i].buffer, 1, lines[i].length, f);
        if (i < lineCount - 1) fwrite("\r\n", 1, 2, f);
    }
    fclose(f);
    return 1;
}

void savefile() {
    if (!filepath) {
        saveas(); // never saved/opened yet: ask for a location
        return;
    }
    writeFile(filepath);
}

void saveas() {
    nfdchar_t *outPath = NULL;
    if (NFD_SaveDialog(&outPath, NULL, 0, NULL, filename) != NFD_OKAY) {
        return; // cancelled or failed
    }
    // On some platforms outPath may be NULL when the default name was kept.
    const char *target = outPath ? outPath : filename;
    if (writeFile(target)) {
        setFilepath(target);
        setFilename(baseNameOf(target));
    }
    if (outPath) NFD_FreePath(outPath);
}
