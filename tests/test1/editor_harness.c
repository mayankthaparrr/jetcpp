// Headless test harness for Jet C++ editor logic.
// Links the REAL ui_textarea.c / ui_scroll.c / undo.c / ui_ribbonEvents.c
// against fake raylib + NFD functions, then drives frames with scripted input.
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include <nfd.h>
#include "ui/ui_textarea.h"
#include "ui/ui_ribbonEvents.h"
#include "func/undo.h"
#include "ui/ui_syntax.h"

extern float vertscroll, horscroll;
extern float startX, startY;

#include "fakes.inc"

// ---------------------------------------------------------------- driver
static Font font;

static void frame(void) {
    drawCount = 0;
    borders(25.0f, "T.C", font);
    textstuff(font, 25.0f);
    memset(pressed, 0, sizeof pressed);
    charCount = charPos = 0;
    mPressed = mReleased = 0;
    now += 1.0 / 60.0;
}
static void key(int k) { pressed[k] = 1; frame(); }
static void keyWith(int mod, int k) { down[mod] = 1; pressed[k] = 1; frame(); down[mod] = 0; }
static void keyWith2(int m1, int m2, int k) { down[m1] = down[m2] = 1; pressed[k] = 1; frame(); down[m1] = down[m2] = 0; }
static void type(const char *s) {
    for (int i = 0; s[i];) { int n; charQueue[charCount++] = GetCodepointNext(&s[i], &n); i += n; }
    frame();
}
static void clickAt(float x, float y, int holdFrames) {
    mpos = (Vector2){x, y};
    mPressed = mDown = 1; frame();
    for (int i = 0; i < holdFrames; i++) frame();
    mDown = 0; mReleased = 1; frame();
}
static void setDoc(const char *text) {
    undo_reset();
    for (int i = 0; i < lineCount; i++) free(lines[i].buffer);
    free(lines); lines = NULL; lineCount = 0;
    const char *s = text;
    for (;;) {
        const char *nl = strchr(s, '\n');
        int len = nl ? (int)(nl - s) : (int)strlen(s);
        addLine(lineCount);
        Line *l = &lines[lineCount - 1];
        free(l->buffer);
        l->buffer = malloc(len + 1);
        memcpy(l->buffer, s, len);
        l->buffer[len] = 0;
        l->length = len; l->capacity = len + 1;
        if (!nl) break;
        s = nl + 1;
    }
    cursorLine = cursorColumn = 0; selecting = 0; scrollLine = 0; scrollX = 0;
    contentWidthDirty = 1;
    frame(); frame();
}
static const char *doc(void) {
    static char buf[65536];
    int p = 0;
    for (int i = 0; i < lineCount; i++) {
        p += snprintf(buf + p, sizeof buf - p, "%s%s", lines[i].buffer, i < lineCount - 1 ? "\n" : "");
    }
    return buf;
}

static int fails = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define CHECK_DOC(expect) CHECK(strcmp(doc(), expect) == 0, "doc=[%s] expected=[%s]", doc(), expect)
#define CHECK_CUR(l, c) CHECK(cursorLine == (l) && cursorColumn == (c), "cursor=(%d,%d) expected=(%d,%d)", cursorLine, cursorColumn, l, c)
#define CTRL KEY_LEFT_CONTROL
#define SHIFT KEY_LEFT_SHIFT

static int visibleRows(void) { return (int)((ScreenRect.height - 40) / lineHeight); }

// Caret x relative to the view, read back from the "|" the editor drew.
static float visualCol_probe(void) {
    frame();
    for (int i = 0; i < drawCount; i++)
        if (!strcmp(draws[i].text, "|")) return draws[i].x - startX;
    return -1;
}

// colour of the first drawn run whose text is exactly `run` (from the last frame)
static int colourOf(const char *run, Color *out) {
    for (int i = 0; i < drawCount; i++) {
        if (!strcmp(draws[i].text, run)) { *out = draws[i].tint; return 1; }
    }
    return 0;
}
static int drawnIn(const char *run, int kind) {
    Color c, want = syntaxColor(kind);
    return colourOf(run, &c) && c.r == want.r && c.g == want.g && c.b == want.b;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0); // see results even if the code under test crashes
    addLine(0); // like main.c: the editor always has at least one line
    frame();

    // ---- S1: plain Up/Down/Home/End end the selection instead of extending it
    setDoc("hello\nworld\nfoo");
    keyWith(SHIFT, KEY_RIGHT); keyWith(SHIFT, KEY_RIGHT); keyWith(SHIFT, KEY_RIGHT);
    CHECK(selecting == 1, "shift+right should select");
    key(KEY_DOWN);
    CHECK(selecting == 0, "plain Down should end the selection");
    CHECK_CUR(1, 3);
    type("X");
    CHECK_DOC("hello\nworXld\nfoo");

    setDoc("hello\nworld");
    key(KEY_END); keyWith(SHIFT, KEY_LEFT); keyWith(SHIFT, KEY_LEFT);
    key(KEY_HOME);
    CHECK(selecting == 0, "plain Home should end the selection");
    type("Z");
    CHECK_DOC("Zhello\nworld");

    // ---- S2: Enter / Tab replace the selection, one undo step
    setDoc("hello\nworld");
    keyWith(SHIFT, KEY_RIGHT); keyWith(SHIFT, KEY_RIGHT); keyWith(SHIFT, KEY_RIGHT);
    key(KEY_ENTER);
    CHECK_DOC("\nlo\nworld");
    CHECK_CUR(1, 0);
    CHECK(selecting == 0, "Enter should consume the selection");
    keyWith(CTRL, KEY_Z);
    CHECK_DOC("hello\nworld");

    setDoc("hello");
    keyWith(SHIFT, KEY_RIGHT); keyWith(SHIFT, KEY_RIGHT);
    key(KEY_TAB);
    CHECK_DOC("\tllo");
    CHECK_CUR(0, 1);
    keyWith(CTRL, KEY_Z);
    CHECK_DOC("hello");

    // multi-line selection + Enter
    setDoc("abc\ndef\nghi");
    key(KEY_RIGHT); keyWith(SHIFT, KEY_DOWN);  // (0,1)-(1,1)
    key(KEY_ENTER);
    CHECK_DOC("a\nef\nghi");
    keyWith(CTRL, KEY_Z);
    CHECK_DOC("abc\ndef\nghi");

    // ---- W1: word jumps stop at line edges
    setDoc("abc\nfoo bar");
    key(KEY_DOWN); key(KEY_END);
    CHECK_CUR(1, 7);
    keyWith(CTRL, KEY_LEFT);  CHECK_CUR(1, 4);
    keyWith(CTRL, KEY_LEFT);  CHECK_CUR(1, 0);
    keyWith(CTRL, KEY_LEFT);  CHECK_CUR(0, 3);
    keyWith(CTRL, KEY_RIGHT); CHECK_CUR(1, 0);
    keyWith(CTRL, KEY_RIGHT); CHECK_CUR(1, 3);
    keyWith(CTRL, KEY_RIGHT); CHECK_CUR(1, 7);
    keyWith(CTRL, KEY_RIGHT); CHECK_CUR(1, 7);

    setDoc("abc\nfoo");
    key(KEY_DOWN); key(KEY_END);
    keyWith(CTRL, KEY_BACKSPACE);
    CHECK_DOC("abc\n");
    CHECK_CUR(1, 0);
    keyWith(CTRL, KEY_BACKSPACE);
    CHECK_DOC("abc");
    CHECK_CUR(0, 3);
    keyWith(CTRL, KEY_Z); CHECK_DOC("abc\n");
    keyWith(CTRL, KEY_Z); CHECK_DOC("abc\nfoo");

    setDoc("foo bar\nxyz");
    keyWith(CTRL, KEY_DELETE); CHECK_DOC(" bar\nxyz");
    keyWith(CTRL, KEY_DELETE); CHECK_DOC("\nxyz");
    keyWith(CTRL, KEY_DELETE); CHECK_DOC("xyz");
    keyWith(CTRL, KEY_Z); keyWith(CTRL, KEY_Z); keyWith(CTRL, KEY_Z);
    CHECK_DOC("foo bar\nxyz");

    // ---- N1: Home/End are line-local; Ctrl+Home/End are document-wide
    setDoc("aaa\nbbbb\ncc");
    key(KEY_DOWN); key(KEY_END); CHECK_CUR(1, 4);
    key(KEY_HOME); CHECK_CUR(1, 0);
    key(KEY_END);  CHECK_CUR(1, 4);
    keyWith(CTRL, KEY_HOME); CHECK_CUR(0, 0);
    keyWith(CTRL, KEY_END);  CHECK_CUR(2, 2);

    // Shift+Home selects to line start
    setDoc("hello");
    key(KEY_END);
    keyWith(SHIFT, KEY_HOME);
    CHECK(selecting == 1 && selectionColumn == 5 && cursorColumn == 0, "Shift+Home should select");
    type("X");
    CHECK_DOC("X");

    // ---- U1: UTF-8 typing / navigation / deletion
    setDoc("");
    type("\xC3\xA9");             // é
    CHECK(lines[0].length == 2 && memcmp(lines[0].buffer, "\xC3\xA9", 2) == 0, "é should be stored as 2 UTF-8 bytes");
    CHECK_CUR(0, 2);
    type("\xE2\x82\xAC");         // €
    CHECK(lines[0].length == 5, "€ should add 3 bytes (len=%d)", lines[0].length);
    key(KEY_BACKSPACE);
    CHECK_DOC("\xC3\xA9");
    key(KEY_BACKSPACE);
    CHECK_DOC("");

    setDoc("a\xE2\x80\x94" "b");  // a—b
    key(KEY_RIGHT); CHECK_CUR(0, 1);
    key(KEY_RIGHT); CHECK_CUR(0, 4);
    key(KEY_RIGHT); CHECK_CUR(0, 5);
    key(KEY_LEFT);  CHECK_CUR(0, 4);
    key(KEY_LEFT);  CHECK_CUR(0, 1);
    key(KEY_DELETE);
    CHECK_DOC("ab");
    keyWith(CTRL, KEY_Z);
    CHECK_DOC("a\xE2\x80\x94" "b");

    // caret x after an em-dash: one column per codepoint
    setDoc("a\xE2\x80\x94" "b");
    key(KEY_END);
    frame();
    {
        // cursor() draws the bar 3px left of the column so it sits between characters
        float cx = -1;
        float want = startX - scrollX + 3 * (ADV + 0.8f) - 3;
        for (int i = 0; i < drawCount; i++) if (!strcmp(draws[i].text, "|")) cx = draws[i].x;
        CHECK(fabsf(cx - want) < 0.01f, "caret x=%f expected %f", cx, want);
    }

    // ---- T1: tab stops (render + Up/Down keep the screen column)
    setDoc("ab\tcd");
    frame();
    {
        float xab = -1, xcd = -1;
        for (int i = 0; i < drawCount; i++) {
            if (!strcmp(draws[i].text, "ab")) xab = draws[i].x;
            if (!strcmp(draws[i].text, "cd")) xcd = draws[i].x;
        }
        CHECK(xab >= 0 && fabsf((xcd - xab) - 4 * (ADV + 0.8f)) < 0.01f,"tab run: ab@%f cd@%f", xab, xcd);
    }
    setDoc("\tx\nabcdefgh");
    key(KEY_DOWN); key(KEY_RIGHT); key(KEY_RIGHT); key(KEY_RIGHT); key(KEY_RIGHT);
    CHECK_CUR(1, 4);
    key(KEY_UP);   CHECK_CUR(0, 1);   // screen col 4 = just after the tab
    key(KEY_DOWN); CHECK_CUR(1, 4);
    key(KEY_LEFT); key(KEY_LEFT);
    key(KEY_UP);   CHECK_CUR(0, 0);   // screen col 2 is inside the tab: before it

    // ---- O1 + paste return value
    setDoc("hello");
    key(KEY_RIGHT); key(KEY_RIGHT);
    clip = "X\r\nY\nZ";
    keyWith(CTRL, KEY_V);
    CHECK_DOC("heX\nY\nZllo");
    CHECK_CUR(2, 1);
    keyWith(CTRL, KEY_Z);
    CHECK_DOC("hello");

    setDoc("a");
    key(KEY_END); type("b"); key(KEY_LEFT);
    clip = "";
    keyWith(CTRL, KEY_V);          // empty paste: must not push an empty undo step
    keyWith(CTRL, KEY_Z);
    CHECK_DOC("a");

    // ---- M1: an open menu swallows text clicks
    setDoc("hello\nworld");
    fakeMenuOpen = 1;
    clickAt(startX + 2 * (ADV + 0.8f) + 1, startY + lineHeight + 5, 3);
    CHECK_CUR(0, 0);
    fakeMenuOpen = 0;
    clickAt(startX + 2 * (ADV + 0.8f) + 1, startY + lineHeight + 5, 0);
    CHECK_CUR(1, 2);

    // ---- K3/N3: caret stays in view after Enter at the bottom, held Right, Ctrl+Down
    {
        char big[4096] = "";
        for (int i = 0; i < 30; i++) { char t[16]; sprintf(t, "l%d\n", i); strcat(big, t); }
        big[strlen(big) - 1] = 0;
        setDoc(big);
        keyWith(CTRL, KEY_END);
        frame();
        for (int i = 0; i < 5; i++) key(KEY_ENTER);
        frame();
        CHECK(cursorLine >= scrollLine && cursorLine < scrollLine + visibleRows(),
              "after Enter: cursorLine=%d scrollLine=%d visible=%d", cursorLine, scrollLine, visibleRows());
        keyWith(CTRL, KEY_UP);
        frame();
        CHECK(scrollLine == 0, "Ctrl+Up should scroll to top (scrollLine=%d)", scrollLine);
        keyWith(CTRL, KEY_DOWN);
        frame();
        CHECK(cursorLine >= scrollLine && cursorLine < scrollLine + visibleRows(),
              "after Ctrl+Down: cursorLine=%d scrollLine=%d", cursorLine, scrollLine);
    }
    {
        // K3 proper: reach the bottom with Down (which always scrolled), then Enter
        char big[4096] = "";
        for (int i = 0; i < 60; i++) { char t[16]; sprintf(t, "l%d\n", i); strcat(big, t); }
        big[strlen(big) - 1] = 0;
        setDoc(big);
        for (int i = 0; i < visibleRows() + 3; i++) key(KEY_DOWN);
        int before = scrollLine;
        for (int i = 0; i < 5; i++) key(KEY_ENTER);
        frame();
        CHECK(scrollLine > before && cursorLine < scrollLine + visibleRows(),
              "Enter at bottom: cursorLine=%d scrollLine=%d (was %d) visible=%d", cursorLine, scrollLine, before, visibleRows());
    }
    {
        char wide[300];
        memset(wide, 'a', 200); wide[200] = 0;
        setDoc(wide);
        for (int i = 0; i < 150; i++) key(KEY_RIGHT);
        frame();
        float caret = 150 * (ADV + 0.8f) - scrollX;
        CHECK(caret >= 0 && caret <= ScreenRect.width - 20 + 0.01f, "Right x150: caret at %f, scrollX=%f", caret, scrollX);

        // a real HELD key: one press, then auto-repeat (no new presses)
        setDoc(wide);
        pressed[KEY_RIGHT] = 1; down[KEY_RIGHT] = 1; frame();
        for (int i = 0; i < 300; i++) frame();       // 5s of repeat
        down[KEY_RIGHT] = 0; frame();
        caret = visualCol_probe();
        CHECK(cursorColumn > 60, "held Right should auto-repeat (col=%d)", cursorColumn);
        CHECK(caret >= 0 && caret <= ScreenRect.width - 20 + 0.01f, "held Right: caret at %f, scrollX=%f", caret, scrollX);
    }

    // ---- Select All keeps the view; bottom-strip click doesn't run away
    {
        char big[8192] = "";
        for (int i = 0; i < 100; i++) strcat(big, "line\n");
        big[strlen(big) - 1] = 0;
        setDoc(big);
        keyWith(CTRL, KEY_A);
        frame(); frame();
        CHECK(scrollLine == 0, "Ctrl+A scrolled to %d", scrollLine);
        CHECK(selecting && cursorLine == 99, "Ctrl+A should select all");
        key(KEY_ESCAPE);
        clickAt(startX + 5, ScreenRect.y + ScreenRect.height - 3, 20);
        CHECK(scrollLine == 0, "click in bottom strip scrolled to %d", scrollLine);

        // ---- K7: scrollbar visibility decided on the same frame
        CHECK(vertscroll == 15, "100 lines should need the vertical bar (vertscroll=%f)", vertscroll);
        setDoc("short");
        CHECK(vertscroll == 0 && horscroll == 0, "short doc: v=%f h=%f", vertscroll, horscroll);
        char wide[400];
        memset(wide, 'w', 300); wide[300] = 0;
        setDoc(wide);
        CHECK(horscroll == 10, "300 cols should need the horizontal bar (horscroll=%f)", horscroll);
    }

    // ---- regressions: plain Backspace/Delete merges, typing over selection = 1 undo step
    setDoc("ab\ncd");
    key(KEY_DOWN);
    key(KEY_BACKSPACE); CHECK_DOC("abcd"); CHECK_CUR(0, 2);
    key(KEY_DELETE);    CHECK_DOC("abd");
    setDoc("ab\ncd");
    key(KEY_END); key(KEY_DELETE); CHECK_DOC("abcd");
    setDoc("hello");
    for (int i = 0; i < 5; i++) keyWith(SHIFT, KEY_RIGHT);
    type("abc"); key(KEY_LEFT);
    keyWith(CTRL, KEY_Z);
    CHECK_DOC("hello");
    keyWith2(CTRL, SHIFT, KEY_Z);
    CHECK_DOC("abc");

    // ---- highlighting: the scanner (filename is noname.c here, so it's on)
    {
        unsigned char k[64];
        const char *s = "int x = 0x1F; // hi";
        syntaxLine(s, (int)strlen(s), 0, k);
        CHECK(k[0] == SYN_KEYWORD && k[2] == SYN_KEYWORD && k[4] == SYN_TEXT, "int is a keyword, x is text");
        CHECK(k[8] == SYN_NUMBER && k[11] == SYN_NUMBER && k[12] == SYN_TEXT, "0x1F is a number");
        CHECK(k[14] == SYN_COMMENT && k[18] == SYN_COMMENT, "// to the end is a comment");
        s = "a = \"/* not \\\" a comment\"; b";
        CHECK(syntaxLine(s, (int)strlen(s), 0, k) == 0, "/* inside a string doesn't open a comment");
        CHECK(k[4] == SYN_STRING && k[(int)strlen(s) - 4] == SYN_STRING && k[(int)strlen(s) - 1] == SYN_TEXT, "the string ends at the unescaped quote");
        s = "  #include <stdio.h>";
        syntaxLine(s, (int)strlen(s), 0, k);
        CHECK(k[2] == SYN_PREPROC && k[11] == SYN_PREPROC && k[19] == SYN_PREPROC, "#include <file>: the whole line is preprocessor, like turbo");
        s = "#define MAX 10 /* top */ // note";
        syntaxLine(s, (int)strlen(s), 0, k);
        CHECK(k[8] == SYN_PREPROC && k[12] == SYN_PREPROC && k[15] == SYN_COMMENT && k[25] == SYN_COMMENT, "#define line is preprocessor except its comments");
        s = "a @ b ` c";
        syntaxLine(s, (int)strlen(s), 0, k);
        CHECK(k[2] == SYN_ILLEGAL && k[6] == SYN_ILLEGAL && k[0] == SYN_TEXT, "@ and ` are illegal characters");
        s = "x = \"@\";";
        syntaxLine(s, (int)strlen(s), 0, k);
        CHECK(k[5] == SYN_STRING, "@ inside a string is fine");
        s = "still */ x";
        CHECK(syntaxLine(s, (int)strlen(s), 1, k) == 0 && k[0] == SYN_COMMENT && k[7] == SYN_COMMENT && k[9] == SYN_TEXT, "a comment from the line before ends at */");
        s = "char c = '\"'; int y;";
        syntaxLine(s, (int)strlen(s), 0, k);
        CHECK(k[9] == SYN_STRING && k[11] == SYN_STRING && k[14] == SYN_KEYWORD, "a '\"' char doesn't start a string");
    }
    // drawn colours, incl. a comment spanning lines and it updating on edit + undo
    setDoc("int x = 0x1F; // hi");
    frame();
    CHECK(drawnIn("int", SYN_KEYWORD) && drawnIn("0x1F", SYN_NUMBER) && drawnIn("// hi", SYN_COMMENT), "drawn colours for int / 0x1F / comment");
    setDoc("a /* b\nc\nd */ e");
    frame();
    CHECK(drawnIn("c", SYN_COMMENT), "a line inside /* */ is a comment");
    CHECK(drawnIn(" e", SYN_TEXT), "after */ is plain text");
    key(KEY_RIGHT); key(KEY_RIGHT); key(KEY_DELETE); key(KEY_DELETE); // remove the /*
    frame();
    CHECK(drawnIn("c", SYN_TEXT), "deleting /* makes the next lines plain again");
    keyWith(CTRL, KEY_Z);
    frame();
    CHECK(drawnIn("c", SYN_COMMENT), "undo brings the comment back");
    // tabs and colours together: runs still land on tab stops
    setDoc("\tint\tx;");
    frame();
    {
        float xi = -1, xx = -1;
        for (int i = 0; i < drawCount; i++) {
            if (!strcmp(draws[i].text, "int")) xi = draws[i].x;
            if (!strcmp(draws[i].text, "x;")) xx = draws[i].x;
        }
        CHECK(xi >= 0 && fabsf((xx - xi) - 4 * (ADV + 0.8f)) < 0.01f, "int at col 4, x at col 8 (int@%f x@%f)", xi, xx);
        CHECK(drawnIn("int", SYN_KEYWORD), "int after a tab is still yellow");
    }
    // clangd's tokens go on top, but never inside comments; stale ones are skipped
    setDoc("size_t n = strlen(s); // strlen");
    fakeTokens[0] = (LspToken){ 0, 0, 6, 0 };   // size_t: type
    fakeTokens[1] = (LspToken){ 0, 11, 6, 1 };  // strlen: function
    fakeTokens[2] = (LspToken){ 0, 25, 6, 1 };  // strlen in the comment
    fakeTokens[3] = (LspToken){ 0, 40, 5, 0 };  // past the end (text changed since)
    fakeTokens[4] = (LspToken){ 1, 0, 3, 2 };   // a line that doesn't exist
    fakeTokenCount = 5;
    frame();
    CHECK(drawnIn("size_t", SYN_TYPE), "size_t from clangd is a type");
    CHECK(drawnIn("strlen", SYN_FUNCTION), "strlen from clangd is a function");
    CHECK(drawnIn("// strlen", SYN_COMMENT), "clangd can't recolour a comment");
    fakeTokenCount = 0;
    // a non-code file stays white
    {
        FILE *f = fopen("harness_notes.md", "wb"); fputs("int x; // not code here\n\tint tabbed;\n", f); fclose(f);
        loadFileAtPath("harness_notes.md");
        frame();
        Color c1 = { 0 }, c2 = { 0 };
        CHECK(colourOf("int x; // not code here", &c1) && c1.r == 255 && c1.g == 255 && c1.b == 255, "a .md file is drawn white in one piece");
        CHECK(colourOf("int tabbed;", &c2) && c2.r == 255 && c2.g == 255 && c2.b == 255, "a .md line with a tab is white too");
        remove("harness_notes.md");
    }

    // ---- K8 / F1: file I/O via the real ui_ribbonEvents.c
    {
        const char *p = "harness_tmp.c";
        FILE *f = fopen(p, "wb"); fputs("one\r\n\ttwo\r\nthr\xC3\xA9" "e\r\n", f); fclose(f);
        loadFileAtPath(p);
        CHECK_DOC("one\n\ttwo\nthr\xC3\xA9" "e");
        savePath = p;
        saveas();
        f = fopen(p, "rb"); char buf[64] = {0}; size_t n = fread(buf, 1, sizeof buf - 1, f); fclose(f);
        CHECK(n == strlen("one\r\n\ttwo\r\nthr\xC3\xA9" "e") && !strcmp(buf, "one\r\n\ttwo\r\nthr\xC3\xA9" "e"), "saved=[%s]", buf);
        remove(p);
        savePath = "Z:/definitely/missing/dir/x.c";
        saveas(); // must report failure, not crash
        loadFileAtPath("Z:/definitely/missing/dir/x.c"); // missing file: status error, doc unchanged
        CHECK_DOC("one\n\ttwo\nthr\xC3\xA9" "e");
    }

    printf("%d/%d checks passed\n", checks - fails, checks);
    return fails ? 1 : 0;
}
