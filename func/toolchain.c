//this is toolchain.c
//Run menu: compile the open file with clang, then run it or open it in lldb
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "raylib.h"
#include "toolchain.h"
#include "app_spawn.h"
#include "../ui/ui_messages.h"
#include "../ui/ui_ribbonEvents.h"

enum { THEN_NOTHING, THEN_RUN, THEN_DEBUG };

static int building = 0;
static int stopped = 0;
static int thenDo = THEN_NOTHING;
static char exePath[1024];

// clang + lldb ship in toolchain\bin next to the exe. while developing they
// come from the repo's tools folder instead
int findTool(const char *name, char *out, int size) {
    snprintf(out, size, "%stoolchain\\bin\\%s", GetApplicationDirectory(), name);
    if (!FileExists(out)) {
        snprintf(out, size, "%s\\tools\\llvm-mingw\\bin\\%s", JET_SOURCE_DIR, name);
        if (!FileExists(out)) return 0;
    }
    for (char *p = out; *p; p++) {
        if (*p == '/') *p = '\\';
    }
    return 1;
}

static int isCpp(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return 0;
    char ext[8] = "";
    for (int i = 0; dot[i] && i < 7; i++) {
        ext[i] = tolower((unsigned char)dot[i]);
        ext[i + 1] = '\0';
    }
    return !strcmp(ext, ".cpp") || !strcmp(ext, ".cc") || !strcmp(ext, ".cxx") || !strcmp(ext, ".c++");
}

static void build(int then) {
    if (building) {
        messagesLine("Still compiling...");
        return;
    }
    if (!savefile()) return; // Save As got cancelled, or the save failed
    const char *src = currentFilepath();

    messagesClear();
    messagesShow(1);

    int cpp = isCpp(src);
    char cc[1024];
    if (!findTool(cpp ? "clang++.exe" : "clang.exe", cc, sizeof(cc))) {
        messagesLine("Can't find clang. It goes in toolchain\\bin next to Jet C++");
        return;
    }

    // prog.c -> prog.exe, next to the source
    snprintf(exePath, sizeof(exePath), "%s", src);
    char *dot = strrchr(exePath, '.');
    char *slash = strrchr(exePath, '\\');
    if (!slash) slash = strrchr(exePath, '/');
    if (dot && (!slash || dot > slash)) *dot = '\0';
    strncat(exePath, ".exe", sizeof(exePath) - strlen(exePath) - 1);

    killConsole(); // the last run still has the exe open, the linker can't overwrite it

    // -x says the language outright: clang treats a capital .C (like NONAME.C) as c++.
    // c++ needs -static or the exe wants libc++.dll from the toolchain folder
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "\"%s\" -x %s %s -g -O0 -Wall -fno-color-diagnostics \"%s\" -o \"%s\"%s",
             cc, cpp ? "c++" : "c", cpp ? "-std=c++17" : "-std=c11", src, exePath, cpp ? " -static" : "");

    messagesLine(TextFormat("Compiling %s", filename));
    if (!spawnTool(cmd)) {
        messagesLine("Couldn't start clang");
        return;
    }
    building = 1;
    stopped = 0;
    thenDo = then;
}

void compileOnly(void) { build(THEN_NOTHING); }
void compileAndRun(void) { build(THEN_RUN); }
void compileAndDebug(void) { build(THEN_DEBUG); }

static void exeFolder(char *dir, int size) {
    snprintf(dir, size, "%s", exePath);
    char *slash = strrchr(dir, '\\');
    if (!slash) slash = strrchr(dir, '/');
    if (slash) *slash = '\0';
}

static void runProgram(void) {
    char dir[1024], cmd[2048];
    exeFolder(dir, sizeof(dir));
    // "& pause" keeps the window up after the program ends, like turbo's user screen
    snprintf(cmd, sizeof(cmd), "cmd.exe /c \"\"%s\" & pause\"", exePath);
    if (!spawnConsole(cmd, dir)) messagesLine("Couldn't start the program");
}

static void debugProgram(void) {
    char dir[1024], lldb[1024], cmd[2048];
    if (!findTool("lldb.exe", lldb, sizeof(lldb))) {
        messagesLine("Can't find lldb.exe");
        return;
    }
    exeFolder(dir, sizeof(dir));
    snprintf(cmd, sizeof(cmd), "\"%s\" \"%s\"", lldb, exePath);
    if (!spawnConsole(cmd, dir)) {
        messagesLine("Couldn't start lldb");
        return;
    }
    messagesLine("lldb is open in its own window: b main, run, n, s, p x, q");
}

void stopProgram(void) {
    if (building) {
        stopped = 1;
        killTool();
    }
    killConsole();
}

void toolchainUpdate(void) {
    if (!building) return;
    char buf[4096];
    int n;
    while ((n = readTool(buf, sizeof(buf))) > 0) messagesAppend(buf, n);

    int code;
    if (!toolDone(&code)) return;
    while ((n = readTool(buf, sizeof(buf))) > 0) messagesAppend(buf, n); // the last of it
    closeTool();
    building = 0;

    if (stopped) {
        messagesLine("Stopped");
    } else if (code != 0) {
        messagesLine("Failed. Click an error to jump to it");
    } else {
        messagesLine("Success");
        if (thenDo == THEN_RUN) runProgram();
        if (thenDo == THEN_DEBUG) debugProgram();
    }
}
