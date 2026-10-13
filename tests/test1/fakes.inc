// fake raylib + NFD for the harnesses. drawing does nothing (DrawTextEx records
// what it was asked to draw), input comes from the arrays below.
#include <stdarg.h>
#include <io.h>

#define ADV 14.0f
static int down[512], pressed[512];
static int charQueue[64], charCount, charPos;
static Vector2 mpos;
static int mDown, mPressed, mReleased;
static const char *clip = "";
static double now = 0;
int fakeMenuOpen = 0;

typedef struct { char text[256]; float x, y; Color tint; } DrawRec;
static DrawRec draws[512];
static int drawCount;

int GetCodepointNext(const char *text, int *codepointSize) {
    const char *ptr = text;
    int codepoint = 0x3f;
    *codepointSize = 1;
    if (0xf0 == (0xf8 & ptr[0])) {
        if (((ptr[1] & 0xC0) ^ 0x80) || ((ptr[2] & 0xC0) ^ 0x80) || ((ptr[3] & 0xC0) ^ 0x80)) return codepoint;
        codepoint = ((0x07 & ptr[0]) << 18) | ((0x3f & ptr[1]) << 12) | ((0x3f & ptr[2]) << 6) | (0x3f & ptr[3]);
        *codepointSize = 4;
    } else if (0xe0 == (0xf0 & ptr[0])) {
        if (((ptr[1] & 0xC0) ^ 0x80) || ((ptr[2] & 0xC0) ^ 0x80)) return codepoint;
        codepoint = ((0x0f & ptr[0]) << 12) | ((0x3f & ptr[1]) << 6) | (0x3f & ptr[2]);
        *codepointSize = 3;
    } else if (0xc0 == (0xe0 & ptr[0])) {
        if ((ptr[1] & 0xC0) ^ 0x80) return codepoint;
        codepoint = ((0x1f & ptr[0]) << 6) | (0x3f & ptr[1]);
        *codepointSize = 2;
    } else if (0x00 == (0x80 & ptr[0])) {
        codepoint = ptr[0];
        *codepointSize = 1;
    }
    return codepoint;
}
const char *CodepointToUTF8(int codepoint, int *utf8Size) {
    static char utf8[6];
    memset(utf8, 0, 6);
    int size = 0;
    if (codepoint <= 0x7f) { utf8[0] = (char)codepoint; size = 1; }
    else if (codepoint <= 0x7ff) { utf8[0] = (char)(((codepoint >> 6) & 0x1f) | 0xc0); utf8[1] = (char)((codepoint & 0x3f) | 0x80); size = 2; }
    else if (codepoint <= 0xffff) { utf8[0] = (char)(((codepoint >> 12) & 0x0f) | 0xe0); utf8[1] = (char)(((codepoint >> 6) & 0x3f) | 0x80); utf8[2] = (char)((codepoint & 0x3f) | 0x80); size = 3; }
    else { utf8[0] = (char)(((codepoint >> 18) & 0x07) | 0xf0); utf8[1] = (char)(((codepoint >> 12) & 0x3f) | 0x80); utf8[2] = (char)(((codepoint >> 6) & 0x3f) | 0x80); utf8[3] = (char)((codepoint & 0x3f) | 0x80); size = 4; }
    *utf8Size = size;
    return utf8;
}
// Same formula as raylib 5.5: n glyph advances + (n-1) spacings.
Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing) {
    (void)font;
    int n = 0;
    for (int i = 0; text[i];) { int s; GetCodepointNext(&text[i], &s); i += s; n++; }
    return (Vector2){ n ? n * ADV + (n - 1) * spacing : 0, fontSize };
}
void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
    (void)font; (void)fontSize; (void)spacing; (void)tint;
    if (drawCount < 512) {
        snprintf(draws[drawCount].text, sizeof draws[0].text, "%s", text);
        draws[drawCount].x = position.x;
        draws[drawCount].y = position.y;
        draws[drawCount].tint = tint;
        drawCount++;
    }
}
double GetTime(void) { return now; }
int GetScreenWidth(void) { return 1000; }
int GetScreenHeight(void) { return 800; }
bool IsKeyPressed(int k) { return pressed[k]; }
bool IsKeyDown(int k) { return down[k] || pressed[k]; }
// raylib queues only initial presses (not OS key repeats) for GetKeyPressed.
int GetKeyPressed(void) { for (int k = 1; k < 512; k++) if (pressed[k]) return k; return 0; }
int GetCharPressed(void) { return charPos < charCount ? charQueue[charPos++] : 0; }
Vector2 GetMousePosition(void) { return mpos; }
int GetMouseX(void) { return (int)mpos.x; }
int GetMouseY(void) { return (int)mpos.y; }
bool IsMouseButtonPressed(int b) { return b == 0 && mPressed; }
bool IsMouseButtonDown(int b) { return b == 0 && mDown; }
bool IsMouseButtonReleased(int b) { return b == 0 && mReleased; }
static float wheel = 0;
float GetMouseWheelMove(void) { return wheel; }
bool CheckCollisionPointRec(Vector2 p, Rectangle r) { return p.x >= r.x && p.x < r.x + r.width && p.y >= r.y && p.y < r.y + r.height; }
void SetClipboardText(const char *t) { static char buf[4096]; snprintf(buf, sizeof buf, "%s", t); clip = buf; }
const char *GetClipboardText(void) { return clip; }
void BeginScissorMode(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
void EndScissorMode(void) {}
void DrawRectangle(int x, int y, int w, int h, Color c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
void DrawRectangleLinesEx(Rectangle r, float t, Color c) { (void)r; (void)t; (void)c; }
void DrawRectangleRounded(Rectangle r, float a, int s, Color c) { (void)r; (void)a; (void)s; (void)c; }
Color GetColor(unsigned int v) { (void)v; return (Color){0}; }
int ribbonMenuOpen(void) { return fakeMenuOpen; }

void NFD_FreePathU8(nfdu8char_t *p) { free(p); }
nfdresult_t NFD_OpenDialogU8(nfdu8char_t **o, const nfdu8filteritem_t *f, nfdfiltersize_t n, const nfdu8char_t *d) { (void)o; (void)f; (void)n; (void)d; return NFD_CANCEL; }
static const char *savePath = NULL;
nfdresult_t NFD_SaveDialogU8(nfdu8char_t **o, const nfdu8filteritem_t *f, nfdfiltersize_t n, const nfdu8char_t *d, const nfdu8char_t *name) {
    (void)f; (void)n; (void)d; (void)name;
    *o = strdup(savePath);
    return NFD_OKAY;
}

bool FileExists(const char *f) { return _access(f, 0) == 0; }
const char *GetApplicationDirectory(void) { return "C:\\nowhere\\"; } // no toolchain next to the "exe": use the repo's tools
const char *TextFormat(const char *fmt, ...) {
    static char bufs[4][1024]; static int k;
    char *b = bufs[k++ & 3];
    va_list ap; va_start(ap, fmt); vsnprintf(b, 1024, fmt, ap); va_end(ap);
    return b;
}
void DrawRectangleRec(Rectangle r, Color c) { (void)r; (void)c; }
Font GetFontDefault(void) { return (Font){0}; }

// the editor calls these now. stubs so the editor/toolchain tests don't start
// a real clangd. lsp_harness defines REAL_LSP and links func/lsp.c instead
#ifndef REAL_LSP
#include "func/lsp.h"
void lspOpen(const char *path) { (void)path; }
void lspChanged(void) {}
void lspUpdate(void) {}
void lspStop(void) {}
// tests can pretend clangd sent these
static LspToken fakeTokens[16];
static int fakeTokenCount = 0;
const LspToken *lspTokens(int *count) { *count = fakeTokenCount; return fakeTokens; }
const char *lspTypeName(int type) {
    static const char *names[] = { "type", "function", "macro", "variable" };
    return type >= 0 && type < 4 ? names[type] : "?";
}
#endif
