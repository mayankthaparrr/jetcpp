// Runs func/lsp.c against the real clangd in tools/llvm-mingw and prints what
// clangd says about a sample file. Only raylib drawing/input is faked.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "raylib.h"
#include <nfd.h>
#include "ui/ui_textarea.h"
#include "ui/ui_ribbonEvents.h"
#include "func/undo.h"
#include "func/lsp.h"
#include "ui/ui_syntax.h"

__declspec(dllimport) void __stdcall Sleep(unsigned long ms);

#define REAL_LSP
#include "fakes.inc"

static Font font;

static int fails = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void writeFile(const char *path, const char *text) {
    FILE *f = fopen(path, "wb"); fputs(text, f); fclose(f);
}
static void setLine(int i, const char *text) {
    free(lines[i].buffer);
    lines[i].buffer = malloc(strlen(text) + 1);
    strcpy(lines[i].buffer, text);
    lines[i].length = (int)strlen(text);
    lines[i].capacity = lines[i].length + 1;
}
static const LspToken *tokenAt(int line, int col) {
    int n;
    const LspToken *t = lspTokens(&n);
    for (int i = 0; i < n; i++) if (t[i].line == line && t[i].col == col) return &t[i];
    return NULL;
}
static const char *kindAt(int line, int col) {
    const LspToken *t = tokenAt(line, col);
    return t ? lspTypeName(t->type) : "-";
}
// keep calling lspUpdate (like the main loop would) until clangd has said this, or 30s
static int waitFor(int line, int col, const char *kind) {
    for (int i = 0; i < 1500; i++) {
        if (!strcmp(kindAt(line, col), kind)) return 1;
        now += 0.02;
        lspUpdate();
        Sleep(20);
    }
    return 0;
}
static int countProcs(const char *image) {
    char cmd[256];
    snprintf(cmd, sizeof cmd, "C:\\Windows\\System32\\tasklist.exe /FI \"IMAGENAME eq %s\" /NH", image);
    FILE *p = _popen(cmd, "r");
    if (!p) return -1;
    char line[512];
    int n = 0;
    while (fgets(line, sizeof line, p)) if (strstr(line, image)) n++;
    _pclose(p);
    return n;
}
static void printTokens(void) {
    int n;
    const LspToken *t = lspTokens(&n);
    printf("   line:col  %-10s kind\n", "text");
    for (int i = 0; i < n; i++) {
        char text[64];
        int len = t[i].len < 63 ? t[i].len : 63;
        if (t[i].line < lineCount && t[i].col + len <= lines[t[i].line].length) {
            memcpy(text, lines[t[i].line].buffer + t[i].col, len);
            text[len] = '\0';
        } else {
            strcpy(text, "?");
        }
        printf("   %4d:%-4d %-10s %s\n", t[i].line + 1, t[i].col, text, lspTypeName(t[i].type));
    }
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    addLine(0);
    char dir[1024];
    snprintf(dir, sizeof dir, "%s\\lsp test", argc > 1 ? argv[1] : ".");
    _mkdir(argc > 1 ? argv[1] : ".");
    _mkdir(dir);
    int clangdBefore = countProcs("clangd.exe");

    // ---- 1. what clangd says about a small C file
    char sample[1100];
    snprintf(sample, sizeof sample, "%s\\sample.c", dir);
    writeFile(sample,
        "#include <string.h>\n"
        "#include <stddef.h>\n"
        "#define TWICE(x) ((x) * 2)\n"
        "size_t count(const char *name) {\n"
        "    size_t n = strlen(name);\n"
        "    return TWICE(n);\n"
        "}\n");
    loadFileAtPath(sample); // the hook in setFilepath() starts clangd
    CHECK(waitFor(4, 15, "function"), "strlen should come back as a function");
    CHECK(countProcs("clangd.exe") == clangdBefore + 1, "one clangd should be running");
    printf("sample.c, as clangd sees it:\n");
    printTokens();
    CHECK(!strcmp(kindAt(3, 0), "type"), "size_t: %s", kindAt(3, 0));
    CHECK(!strcmp(kindAt(3, 7), "function"), "count: %s", kindAt(3, 7));
    CHECK(!strcmp(kindAt(3, 25), "parameter"), "name: %s", kindAt(3, 25));
    CHECK(!strcmp(kindAt(4, 11), "variable"), "n: %s", kindAt(4, 11));
    CHECK(!strcmp(kindAt(5, 11), "macro"), "TWICE: %s", kindAt(5, 11));
    {
        // draw a frame: clangd's tokens should come out in their colours
        drawCount = 0;
        borders(25.0f, "T.C", font);
        textstuff(font, 25.0f);
        Color want = syntaxColor(SYN_FUNCTION), got = { 0 };
        int found = 0;
        for (int i = 0; i < drawCount; i++) {
            if (!strcmp(draws[i].text, "strlen")) { got = draws[i].tint; found = 1; }
        }
        CHECK(found && got.r == want.r && got.g == want.g && got.b == want.b, "strlen should be drawn in the function colour");
    }

    // ---- 2. edit the text: tokens follow once typing pauses
    setLine(4, "    size_t total = strlen(name);");
    setLine(5, "    return TWICE(total);");
    contentWidthDirty = 1;         // what every edit does
    borders(25.0f, "T.C", font);   // the hook in measureTextMetrics() calls lspChanged()
    CHECK(waitFor(4, 19, "function"), "after the edit strlen moves to col 19 (got %s)", kindAt(4, 19));
    const LspToken *total = tokenAt(4, 11);
    CHECK(total && total->len == 5 && !strcmp(lspTypeName(total->type), "variable"), "total should be a 5-byte variable");

    // ---- 3. columns are bytes, not UTF-16 units: a 2-byte é before a name
    char utf[1100];
    snprintf(utf, sizeof utf, "%s\\utf8.c", dir);
    writeFile(utf, "#include <string.h>\nvoid f(void) { const char *s = \"\xC3\xA9\"; size_t k = strlen(s); (void)k; }\n");
    loadFileAtPath(utf);
    int kCol = (int)(strstr(lines[1].buffer, "size_t k") - lines[1].buffer) + 7;
    CHECK(waitFor(1, kCol, "variable"), "k should be at byte column %d (got %s)", kCol, kindAt(1, kCol));

    // ---- 4. NONAME.C style capital .C is still C: 'new' is a plain variable
    char upper[1100];
    snprintf(upper, sizeof upper, "%s\\UPPER.C", dir);
    writeFile(upper, "int main(void) { int new = 1; return new - 1; }\n");
    loadFileAtPath(upper);
    CHECK(waitFor(0, 21, "variable"), "'new' in UPPER.C should be a C variable (got %s)", kindAt(0, 21));

    // ---- 5. a .cpp file restarts clangd in C++ mode
    char cpp[1100];
    snprintf(cpp, sizeof cpp, "%s\\hello.cpp", dir);
    writeFile(cpp, "#include <string>\nstd::string greet() { return \"hi\"; }\n");
    loadFileAtPath(cpp);
    CHECK(waitFor(1, 12, "function"), "greet should be a function (got %s)", kindAt(1, 12));
    CHECK(!strcmp(kindAt(1, 0), "namespace"), "std: %s", kindAt(1, 0));
    CHECK(countProcs("clangd.exe") == clangdBefore + 1, "still only one clangd after the restart");

    // ---- 6. a non-code file stops clangd
    char readme[1100];
    snprintf(readme, sizeof readme, "%s\\README.md", dir);
    writeFile(readme, "# hi\n");
    loadFileAtPath(readme);
    Sleep(500);
    int n;
    lspTokens(&n);
    CHECK(n == 0, "no tokens for a .md file");
    CHECK(countProcs("clangd.exe") == clangdBefore, "clangd should be stopped");

    printf("%d/%d checks passed\n", checks - fails, checks);
    return fails ? 1 : 0;
}
