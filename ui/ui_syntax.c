//this is ui_syntax.c
//colours for the editor, turbo c++ style. this part only looks at characters:
//keywords, comments, strings, numbers, #include. clangd adds types/functions/macros on top (lsp.c)
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ui_syntax.h"
#include "ui_ribbonEvents.h"

// change colours here. turbo c++ 3.0's defaults, in the EGA palette it used.
// turbo didn't know types/functions/macros, those get other EGA colours
Color syntaxColor(int kind) {
    switch (kind) {
        case SYN_KEYWORD:  return (Color){ 255, 255, 255, 255 }; // white
        case SYN_COMMENT:  return (Color){ 170, 170, 170, 255 }; // light gray
        case SYN_STRING:   return (Color){  85, 255, 255, 255 }; // light cyan
        case SYN_NUMBER:   return (Color){  85, 255, 255, 255 }; // light cyan
        case SYN_PREPROC:  return (Color){  85, 255,  85, 255 }; // light green
        case SYN_ILLEGAL:  return (Color){ 255,  85,  85, 255 }; // light red
        case SYN_TYPE:     return (Color){ 255,  85, 255, 255 }; // light magenta
        case SYN_FUNCTION: return (Color){   0, 170, 170, 255 }; // cyan
        case SYN_MACRO:    return (Color){  85, 255,  85, 255 }; // light green, like #define
        default:           return (Color){ 255, 255,  85, 255 }; // yellow: names + symbols
    }
}

static const char *keywords[] = {
    "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn",
    "_Static_assert", "_Thread_local", "alignas", "alignof", "asm", "auto", "bool", "break", "case",
    "catch", "char", "char16_t", "char32_t", "char8_t", "class", "co_await", "co_return", "co_yield",
    "concept", "const", "const_cast", "consteval", "constexpr", "constinit", "continue", "decltype",
    "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern",
    "false", "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
    "new", "noexcept", "nullptr", "operator", "private", "protected", "public", "register",
    "reinterpret_cast", "requires", "restrict", "return", "short", "signed", "sizeof", "static",
    "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw",
    "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
    "volatile", "wchar_t", "while"
};
static int sorted = 0;

static int compareWords(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static int isKeyword(const char *word, int len) {
    if (len > 31) return 0;
    if (!sorted) {
        qsort(keywords, sizeof(keywords) / sizeof(keywords[0]), sizeof(keywords[0]), compareWords);
        sorted = 1;
    }
    char w[32];
    memcpy(w, word, len);
    w[len] = '\0';
    const char *key = w;
    return bsearch(&key, keywords, sizeof(keywords) / sizeof(keywords[0]), sizeof(keywords[0]), compareWords) != NULL;
}

static int wordStart(char c) {
    return isalpha((unsigned char)c) || c == '_' || (unsigned char)c >= 0x80;
}

static int wordByte(char c) {
    return isalnum((unsigned char)c) || c == '_' || (unsigned char)c >= 0x80;
}

int syntaxLine(const char *text, int len, int inComment, unsigned char *kinds) {
    // a # as the first thing on the line makes it a preprocessor line
    int first = 0;
    while (first < len && (text[first] == ' ' || text[first] == '\t')) first++;
    int preproc = !inComment && first < len && text[first] == '#';

    int i = 0;
    while (i < len) {
        int start = i;
        int kind = SYN_TEXT;
        char c = text[i];
        if (inComment) {
            while (i < len && !(text[i] == '*' && i + 1 < len && text[i + 1] == '/')) i++;
            if (i < len) {
                i += 2;
                inComment = 0;
            }
            kind = SYN_COMMENT;
        } else if (c == '/' && i + 1 < len && text[i + 1] == '/') {
            i = len;
            kind = SYN_COMMENT;
        } else if (c == '/' && i + 1 < len && text[i + 1] == '*') {
            i += 2;
            inComment = 1;
            kind = SYN_COMMENT;
        } else if (c == '"' || c == '\'') {
            i++;
            while (i < len && text[i] != c) {
                if (text[i] == '\\' && i + 1 < len) i++; // skip the escaped char, like \"
                i++;
            }
            if (i < len) i++;
            kind = SYN_STRING;
        } else if (preproc && i == first) {
            // the # and the word after it. after #include, <file> is a string
            i++;
            while (i < len && (text[i] == ' ' || text[i] == '\t')) i++;
            int word = i;
            while (i < len && wordByte(text[i])) i++;
            if (kinds) memset(kinds + start, SYN_PREPROC, i - start);
            if (i - word == 7 && !strncmp(text + word, "include", 7)) {
                int open = i;
                while (open < len && (text[open] == ' ' || text[open] == '\t')) open++;
                if (open < len && text[open] == '<') {
                    int close = open;
                    while (close < len && text[close] != '>') close++;
                    if (close < len) close++;
                    if (kinds) {
                        memset(kinds + i, SYN_TEXT, open - i);
                        memset(kinds + open, SYN_STRING, close - open);
                    }
                    i = close;
                }
            }
            continue;
        } else if (isdigit((unsigned char)c) || (c == '.' && i + 1 < len && isdigit((unsigned char)text[i + 1]))) {
            while (i < len && (isalnum((unsigned char)text[i]) || text[i] == '.' || text[i] == '_' || text[i] == '\'')) i++;
            kind = SYN_NUMBER;
        } else if (wordStart(c)) {
            while (i < len && wordByte(text[i])) i++;
            kind = isKeyword(text + start, i - start) ? SYN_KEYWORD : SYN_TEXT;
        } else {
            if (c == '@' || c == '`') kind = SYN_ILLEGAL; // not allowed in C/C++ outside strings
            i++;
        }
        if (kinds) memset(kinds + start, kind, i - start);
    }
    // turbo colours the whole preprocessor line, everything but comments
    if (preproc && kinds) {
        for (int b = 0; b < len; b++) {
            if (kinds[b] != SYN_COMMENT) kinds[b] = SYN_PREPROC;
        }
    }
    return inComment;
}

int syntaxOn(void) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return 0;
    char ext[8] = "";
    for (int i = 0; dot[i] && i < 7; i++) {
        ext[i] = tolower((unsigned char)dot[i]);
        ext[i + 1] = '\0';
    }
    const char *code[] = { ".c", ".h", ".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx", ".c++" };
    for (int i = 0; i < (int)(sizeof(code) / sizeof(code[0])); i++) {
        if (!strcmp(ext, code[i])) return 1;
    }
    return 0;
}

int syntaxKindFromLsp(const char *t) {
    if (!strcmp(t, "type") || !strcmp(t, "class") || !strcmp(t, "enum") || !strcmp(t, "interface") ||
        !strcmp(t, "struct") || !strcmp(t, "typeParameter") || !strcmp(t, "concept")) return SYN_TYPE;
    if (!strcmp(t, "function") || !strcmp(t, "method")) return SYN_FUNCTION;
    if (!strcmp(t, "macro")) return SYN_MACRO;
    return SYN_TEXT; // variables, parameters, fields, namespaces stay white
}
