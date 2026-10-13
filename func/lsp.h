//this is lsp.h
#ifndef ATR_LSP_H
#define ATR_LSP_H

// one name clangd told us about. col and len are in bytes, same as cursorColumn
typedef struct {
    int line, col, len;
    int type; // index into clangd's list of kinds, see lspTypeName()
} LspToken;

void lspOpen(const char *path);        // this file is open now (starts clangd if it isn't running)
void lspChanged(void);                 // the text changed. it gets sent once typing pauses
void lspUpdate(void);                  // every frame: read clangd's replies, send changes
void lspStop(void);                    // stop clangd (on quit, or when the file isn't C/C++)
const LspToken *lspTokens(int *count); // the latest tokens, in order through the file
const char *lspTypeName(int type);     // "type", "function", "variable", "macro", ...

#endif
