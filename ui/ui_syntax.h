//this is ui_syntax.h
#ifndef ATR_UI_SYNTAX_H
#define ATR_UI_SYNTAX_H
#include "raylib.h"

enum {
    SYN_TEXT, SYN_KEYWORD, SYN_COMMENT, SYN_STRING, SYN_NUMBER, SYN_PREPROC, SYN_ILLEGAL,
    SYN_TYPE, SYN_FUNCTION, SYN_MACRO // these three come from clangd
};

// works out what each byte of one line is. inComment = the line starts inside /* */.
// returns 1 if the line ends inside /* */. kinds can be NULL to just track comments
int syntaxLine(const char *text, int len, int inComment, unsigned char *kinds);
int syntaxOn(void);                         // the open file is C/C++
int syntaxKindFromLsp(const char *lspType); // clangd's "type", "function"... -> SYN_*
Color syntaxColor(int kind);

#endif
