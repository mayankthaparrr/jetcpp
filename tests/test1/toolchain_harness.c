// End-to-end test for Run -> Compile/Run/Debug/Stop + the Message pane.
// Real clang, real pipes, real console windows; only raylib drawing/input is faked.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "raylib.h"
#include <nfd.h>
#include "ui/ui_textarea.h"
#include "ui/ui_ribbonEvents.h"
#include "func/undo.h"

__declspec(dllimport) void __stdcall Sleep(unsigned long ms);
extern float terminal_height;

#include "fakes.inc"

// pull in the code under test so its statics are visible
#include "ui/ui_messages.c"
#include "func/toolchain.c"

static Font font;
static void frame(void) {
    drawCount = 0;
    toolchainUpdate();
    borders(25.0f, "T.C", font);
    textstuff(font, 25.0f);
    drawMessages(font, 25.0f);
    memset(pressed, 0, sizeof pressed);
    charCount = charPos = 0;
    mPressed = mReleased = 0;
    wheel = 0;
    now += 1.0 / 60.0;
}

static int fails = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int hasMsg(const char *needle) {
    for (int i = 0; i < msgCount; i++) if (strstr(msgs[i], needle)) return i;
    return -1;
}
static void dumpMsgs(void) { for (int i = 0; i < msgCount; i++) printf("   | %s\n", msgs[i]); }
// run frames until the build finishes (or 60s)
static void waitBuild(void) {
    for (int i = 0; i < 6000 && building; i++) { frame(); Sleep(10); }
    frame();
}
static void writeFile(const char *path, const char *text) {
    FILE *f = fopen(path, "wb"); fputs(text, f); fclose(f);
}
static int procRunning(const char *image) {
    char cmd[256];
    snprintf(cmd, sizeof cmd, "C:\\Windows\\System32\\tasklist.exe /FI \"IMAGENAME eq %s\" /NH | C:\\Windows\\System32\\findstr.exe /I \"%s\" >NUL", image, image);
    return system(cmd) == 0;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    addLine(0);
    frame();
    // test files go in "<work dir>\tc test" (a space on purpose: paths get quoted)
    char dir[1024];
    snprintf(dir, sizeof dir, "%s\\tc test", argc > 1 ? argv[1] : ".");
    _mkdir(argc > 1 ? argv[1] : ".");
    _mkdir(dir);

    // ---- 1. compile errors show up, pane opens, Failed at the end
    char bad[1100];
    snprintf(bad, sizeof bad, "%s\\bad.c", dir);
    writeFile(bad, "#include <stdio.h>\nint main(void) {\n    int x = ;\n    return y;\n}\n");
    loadFileAtPath(bad);
    compileOnly();
    CHECK(messagesOpen(), "pane should open on compile");
    CHECK(building, "clang should be running");
    waitBuild();
    CHECK(!building, "build should finish");
    CHECK(hasMsg("Compiling bad.c") == 0, "first line should be Compiling");
    int e1 = hasMsg(":3:13: error:");
    CHECK(e1 > 0, "error for line 3 col 13 expected");
    CHECK(hasMsg(":4:12: error:") > 0, "error for line 4 expected");
    CHECK(hasMsg("Failed") == msgCount - 1, "last line should be Failed");
    CHECK(terminal_height > 0 && terminalRect.height == terminal_height, "pane should take space (h=%f)", terminal_height);
    if (fails) dumpMsgs();

    // ---- 2. click the error -> caret jumps there
    {
        float top = terminalRect.y + 25.0f + 4;          // title height = fontsize in the fake
        float lh = 25.0f;
        int row = e1 - msgScroll;
        mpos = (Vector2){ terminalRect.x + 60, top + row * lh + 5 };
        mPressed = mDown = 1; frame(); mDown = 0; mReleased = 1; frame();
        CHECK(cursorLine == 2 && cursorColumn == 12, "jump: cursor=(%d,%d) expected (2,12)", cursorLine, cursorColumn);
        // a non-error line does nothing
        cursorLine = 0; cursorColumn = 0;
        mpos = (Vector2){ terminalRect.x + 60, top + 0 * lh + 5 };  // "Compiling bad.c"
        mPressed = mDown = 1; frame(); mDown = 0; mReleased = 1; frame();
        CHECK(cursorLine == 0 && cursorColumn == 0, "clicking a non-error line moved the cursor to (%d,%d)", cursorLine, cursorColumn);
        // menu open: click ignored
        fakeMenuOpen = 1;
        mpos = (Vector2){ terminalRect.x + 60, top + row * lh + 5 };
        mPressed = mDown = 1; frame(); mDown = 0; mReleased = 1; frame();
        fakeMenuOpen = 0;
        CHECK(cursorLine == 0, "click through an open menu should be ignored");
    }

    // ---- 3. wheel over the pane scrolls the pane, not the editor
    {
        for (int i = 0; i < 40; i++) messagesLine("filler");
        int editorBefore = scrollLine;
        mpos = (Vector2){ terminalRect.x + 60, terminalRect.y + 60 };
        wheel = -2; frame();
        CHECK(msgScroll == 6, "pane should scroll 6 lines (got %d)", msgScroll);
        CHECK(scrollLine == editorBefore, "editor should not scroll while over the pane");
    }

    // ---- 4. error in another file -> status, no jump
    {
        char inc[1100], main2[1100];
        snprintf(inc, sizeof inc, "%s\\inc.h", dir);
        snprintf(main2, sizeof main2, "%s\\uses inc.c", dir);
        writeFile(inc, "int broken( {\n");
        writeFile(main2, "#include \"inc.h\"\nint main(void) { return 0; }\n");
        loadFileAtPath(main2);
        compileOnly();
        waitBuild();
        int e = hasMsg("inc.h:1:");
        CHECK(e > 0, "error in inc.h expected");
        if (e > 0) {
            msgScroll = 0;
            float top = terminalRect.y + 25.0f + 4;
            mpos = (Vector2){ terminalRect.x + 60, top + e * 25.0f + 5 };
            cursorLine = 1; cursorColumn = 3;
            mPressed = mDown = 1; frame(); mDown = 0; mReleased = 1; frame();
            CHECK(cursorLine == 1 && cursorColumn == 3, "other-file error must not move the caret");
        }
    }

    // ---- 5. NONAME.C: compile asks Save As first; cancelling Save As = no compile
    {
        char named[1100];
        snprintf(named, sizeof named, "%s\\saved now.c", dir);
        newfile();
        free(lines[0].buffer);
        lines[0].buffer = strdup("int main(void) { return 0; }");
        lines[0].length = (int)strlen(lines[0].buffer); lines[0].capacity = lines[0].length + 1;
        savePath = named;
        compileOnly();
        waitBuild();
        CHECK(currentFilepath() && !strcmp(currentFilepath(), named), "Save As path should be used");
        CHECK(hasMsg("Success") >= 0, "noname compile should succeed");
        if (hasMsg("Success") < 0) dumpMsgs();
    }

    // ---- 5b. capital .C (NONAME.C is the default name) is still C, not C++
    {
        char upper[1100];
        snprintf(upper, sizeof upper, "%s\\UPPER.C", dir);
        writeFile(upper, "int main(void) { int new = 1; return new - 1; }\n"); // 'new' is fine in C, an error in C++
        loadFileAtPath(upper);
        compileOnly();
        waitBuild();
        CHECK(hasMsg("Success") >= 0, "UPPER.C should compile as C");
        if (hasMsg("Success") < 0) dumpMsgs();
    }

    // ---- 6. Stop while compiling
    {
        char big[1100];
        snprintf(big, sizeof big, "%s\\slow.cpp", dir);
        writeFile(big, "#include <iostream>\n#include <vector>\n#include <map>\n#include <regex>\nint main(){ std::regex r(\"a+\"); std::cout << std::regex_match(\"aaa\", r); }\n");
        loadFileAtPath(big);
        compileOnly();
        CHECK(building, "slow build should be running");
        stopProgram();
        waitBuild();
        CHECK(hasMsg("Stopped") == msgCount - 1, "Stop during build should say Stopped");
        if (hasMsg("Stopped") < 0) dumpMsgs();
    }

    // ---- 7. C++ build: clang++ -static, no libc++.dll needed
    {
        char cpp[1100], exe[1100];
        snprintf(cpp, sizeof cpp, "%s\\hello.cpp", dir);
        snprintf(exe, sizeof exe, "%s\\hello.exe", dir);
        writeFile(cpp, "#include <iostream>\nint main() { std::cout << \"hi\\n\"; }\n");
        loadFileAtPath(cpp);
        compileOnly();
        waitBuild();
        CHECK(hasMsg("Success") >= 0, "c++ compile should succeed");
        if (hasMsg("Success") < 0) dumpMsgs();
        CHECK(FileExists(exe), "hello.exe should exist");
    }

    // ---- 8. Run: program runs in its own window, in its own folder; Stop kills it
    {
        char src[1100], marker[1100];
        snprintf(src, sizeof src, "%s\\runme.c", dir);
        snprintf(marker, sizeof marker, "%s\\ran.txt", dir);
        remove(marker);
        writeFile(src, "#include <stdio.h>\n#include <windows.h>\nint main(void) {\n    FILE *f = fopen(\"ran.txt\", \"w\"); fputs(\"yes\", f); fclose(f);\n    Sleep(60000);\n    return 0;\n}\n");
        loadFileAtPath(src);
        compileAndRun();
        waitBuild();
        CHECK(hasMsg("Success") >= 0, "runme should compile");
        for (int i = 0; i < 300 && !FileExists(marker); i++) Sleep(20);
        CHECK(FileExists(marker), "program should run in its own folder (ran.txt missing)");
        CHECK(procRunning("runme.exe"), "runme.exe should be running");
        stopProgram();
        Sleep(500);
        CHECK(!procRunning("runme.exe"), "Stop should kill runme.exe");
    }

    // ---- 9. Run again while the old window is up: rebuild still works
    {
        char src[1100];
        snprintf(src, sizeof src, "%s\\runme.c", dir);
        loadFileAtPath(src);
        compileAndRun();
        waitBuild();
        Sleep(300);
        compileAndRun();            // exe is still running from the first time
        waitBuild();
        CHECK(hasMsg("Success") >= 0, "second Run should relink fine (old window closed first)");
        if (hasMsg("Success") < 0) dumpMsgs();
        stopProgram();
        Sleep(500);
        CHECK(!procRunning("runme.exe"), "Stop should kill runme.exe again");
    }

    // ---- 10. Debug opens lldb; Stop closes it
    {
        char src[1100];
        snprintf(src, sizeof src, "%s\\runme.c", dir);
        loadFileAtPath(src);
        compileAndDebug();
        waitBuild();
        CHECK(hasMsg("lldb is open") >= 0, "debug should say lldb is open");
        if (hasMsg("lldb is open") < 0) dumpMsgs();
        Sleep(1500);
        CHECK(procRunning("lldb.exe"), "lldb.exe should be running");
        stopProgram();
        Sleep(800);
        CHECK(!procRunning("lldb.exe"), "Stop should close lldb");
    }

    // ---- 11. [x] closes the pane, editor gets its space back
    {
        Vector2 xs = MeasureTextEx(font, "[x]", 25.0f, 0);
        mpos = (Vector2){ terminalRect.x + 15 + xs.x / 2, terminalRect.y + 5 };
        mPressed = mDown = 1; frame(); mDown = 0; mReleased = 1; frame();
        CHECK(!messagesOpen(), "[x] should close the pane");
        CHECK(terminal_height == 0, "editor should get the space back");
    }

    printf("%d/%d checks passed\n", checks - fails, checks);
    return fails ? 1 : 0;
}
