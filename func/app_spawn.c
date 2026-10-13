//
// app_spawn.c — platform-specific "open a new window" support.
//
// See app_spawn.h for the public contract and platform support notes.
//

#ifdef _WIN32

/* ---------------------------- Windows ---------------------------------- */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "app_spawn.h"

int spawnNewInstance(void) {
    return spawnNewInstanceWithFile(NULL);
}

int spawnNewInstanceWithFile(const char *file) {
    // Resolve our own executable path (handles being launched from any
    // working directory). NOTE: Windows-only API.
    char path[4096];
    DWORD len = GetModuleFileNameA(NULL, path, sizeof(path));
    if (len == 0 || len >= (DWORD)sizeof(path)) {
        return 0;
    }

    // Command line: "exe" "file" (quoted; lpApplicationName carries the exe
    // unquoted, but argv[0] must still lead the command line). file==NULL
    // means no argument, i.e. a plain fresh instance.
    char cmdline[8192];
    int n;
    if (file)
        n = snprintf(cmdline, sizeof(cmdline), "\"%s\" \"%s\"", path, file);
    else
        n = snprintf(cmdline, sizeof(cmdline), "\"%s\"", path);
    if (n <= 0 || n >= (int)sizeof(cmdline)) {
        return 0;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return 0;
    }

    // We don't need handles to the child; closing them avoids handle leaks.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 1;
}

// the tool (clang) that's running, and the pipe its output comes out of
static HANDLE toolProcess = NULL;
static HANDLE toolOutput = NULL;
// the program / lldb window. it's a job so Stop kills whatever it started too
static HANDLE consoleJob = NULL;

int spawnTool(const char *cmdline) {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE readEnd, writeEnd;
    if (!CreatePipe(&readEnd, &writeEnd, &sa, 0)) {
        return 0;
    }
    SetHandleInformation(readEnd, HANDLE_FLAG_INHERIT, 0); // only the tool gets the write end

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writeEnd; // stdout and stderr both go in the pipe
    si.hStdError = writeEnd;

    char line[8192];
    snprintf(line, sizeof(line), "%s", cmdline); // CreateProcess wants to write to it
    int ok = CreateProcessA(NULL, line, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(writeEnd); // the tool has its own copy now
    if (!ok) {
        CloseHandle(readEnd);
        return 0;
    }
    CloseHandle(pi.hThread);
    toolProcess = pi.hProcess;
    toolOutput = readEnd;
    return 1;
}

int readTool(char *buf, int size) {
    DWORD waiting = 0, got = 0;
    // only read what's already there, so the window never freezes waiting on clang
    if (!toolOutput || !PeekNamedPipe(toolOutput, NULL, 0, NULL, &waiting, NULL) || waiting == 0) {
        return 0;
    }
    if (waiting > (DWORD)size) waiting = size;
    if (!ReadFile(toolOutput, buf, waiting, &got, NULL)) {
        return 0;
    }
    return (int)got;
}

int toolDone(int *exitCode) {
    if (!toolProcess) {
        *exitCode = -1;
        return 1;
    }
    if (WaitForSingleObject(toolProcess, 0) != WAIT_OBJECT_0) {
        return 0;
    }
    DWORD code = 1;
    GetExitCodeProcess(toolProcess, &code);
    *exitCode = (int)code;
    return 1;
}

void closeTool(void) {
    if (toolProcess) CloseHandle(toolProcess);
    if (toolOutput) CloseHandle(toolOutput);
    toolProcess = NULL;
    toolOutput = NULL;
}

void killTool(void) {
    if (toolProcess) TerminateProcess(toolProcess, 1);
}

int spawnConsole(const char *cmdline, const char *dir) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    char line[8192];
    snprintf(line, sizeof(line), "%s", cmdline);
    // starts paused so it's in the job before it can start anything else
    if (!CreateProcessA(NULL, line, NULL, NULL, FALSE, CREATE_NEW_CONSOLE | CREATE_SUSPENDED, NULL, dir, &si, &pi)) {
        return 0;
    }
    if (consoleJob) CloseHandle(consoleJob); // an older window keeps running, it just can't be stopped anymore
    consoleJob = CreateJobObjectA(NULL, NULL);
    if (consoleJob) AssignProcessToJobObject(consoleJob, pi.hProcess);
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 1;
}

void killConsole(void) {
    if (!consoleJob) return;
    TerminateJobObject(consoleJob, 1);
    CloseHandle(consoleJob);
    consoleJob = NULL;
}

// clangd runs the whole time, with a pipe each way
static HANDLE serverProcess = NULL;
static HANDLE serverIn = NULL;  // we write here, clangd reads it as its stdin
static HANDLE serverOut = NULL; // clangd's stdout, we read here
static HANDLE serverJob = NULL; // kills clangd when Jet C++ goes away, even on a crash

int spawnServer(const char *cmdline) {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE inRead, inWrite, outRead, outWrite;
    if (!CreatePipe(&inRead, &inWrite, &sa, 1 << 20)) {
        return 0;
    }
    if (!CreatePipe(&outRead, &outWrite, &sa, 1 << 20)) {
        CloseHandle(inRead);
        CloseHandle(inWrite);
        return 0;
    }
    SetHandleInformation(inWrite, HANDLE_FLAG_INHERIT, 0); // clangd only gets its own ends
    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
    HANDLE nul = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL); // its log goes nowhere

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = inRead;
    si.hStdOutput = outWrite;
    si.hStdError = nul;

    char line[8192];
    snprintf(line, sizeof(line), "%s", cmdline);
    int ok = CreateProcessA(NULL, line, NULL, NULL, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    CloseHandle(inRead);
    CloseHandle(outWrite);
    if (nul != INVALID_HANDLE_VALUE) CloseHandle(nul);
    if (!ok) {
        CloseHandle(inWrite);
        CloseHandle(outRead);
        return 0;
    }

    serverJob = CreateJobObjectA(NULL, NULL);
    if (serverJob) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit;
        ZeroMemory(&limit, sizeof(limit));
        limit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(serverJob, JobObjectExtendedLimitInformation, &limit, sizeof(limit));
        AssignProcessToJobObject(serverJob, pi.hProcess);
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    serverProcess = pi.hProcess;
    serverIn = inWrite;
    serverOut = outRead;
    return 1;
}

int writeServer(const char *buf, int len) {
    while (len > 0) {
        DWORD wrote = 0;
        if (!serverIn || !WriteFile(serverIn, buf, len, &wrote, NULL)) {
            return 0;
        }
        buf += wrote;
        len -= wrote;
    }
    return 1;
}

int readServer(char *buf, int size) {
    DWORD waiting = 0, got = 0;
    if (!serverOut || !PeekNamedPipe(serverOut, NULL, 0, NULL, &waiting, NULL) || waiting == 0) {
        return 0;
    }
    if (waiting > (DWORD)size) waiting = size;
    if (!ReadFile(serverOut, buf, waiting, &got, NULL)) {
        return 0;
    }
    return (int)got;
}

int serverAlive(void) {
    return serverProcess && WaitForSingleObject(serverProcess, 0) == WAIT_TIMEOUT;
}

void killServer(void) {
    if (serverJob) TerminateJobObject(serverJob, 1);
    else if (serverProcess) TerminateProcess(serverProcess, 1);
    if (serverJob) CloseHandle(serverJob);
    if (serverProcess) CloseHandle(serverProcess);
    if (serverIn) CloseHandle(serverIn);
    if (serverOut) CloseHandle(serverOut);
    serverJob = NULL;
    serverProcess = NULL;
    serverIn = NULL;
    serverOut = NULL;
}

#else /* !_WIN32 */

/* ------------------------------ POSIX ---------------------------------- */
#include <unistd.h>
#include "app_spawn.h"

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
// <limits.h> not strictly needed; readlink needs <unistd.h> (included above).
#endif

// Fills buf with the path of the currently running executable.
// Returns 1 on success, 0 on failure.
static int getSelfPath(char *buf, size_t size) {
#if defined(__APPLE__)
    // NOTE: macOS-only API.
    uint32_t len = (uint32_t)size;
    if (_NSGetExecutablePath(buf, &len) != 0) {
        return 0; // buffer too small
    }
    return 1;
#elif defined(__linux__)
    // NOTE: Linux-only /proc trick; other Unixes don't have this path.
    ssize_t len = readlink("/proc/self/exe", buf, size - 1);
    if (len < 0) {
        return 0;
    }
    buf[len] = '\0';
    return 1;
#else
    // NOTE: not supported on this platform — no portable way to find the
    // running executable's path. Add a case here (e.g. sysctl on BSD, or
    // accept argv[0] as a weaker fallback) if you port ATR to it.
    (void)buf;
    (void)size;
    return 0;
#endif
}

int spawnNewInstance(void) {
    char path[4096];
    if (!getSelfPath(path, sizeof(path))) {
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return 0;
    }
    if (pid == 0) {
        // Child: replace with a fresh copy of this program.
        // argv[0] is set to the path so the child can resolve itself too.
        execl(path, path, (char *)NULL);

        // exec failed — bail out with a distinctive exit code instead of
        // accidentally running a second copy of the parent's code.
        _exit(127);
    }

    // Parent: we intentionally don't wait on the child (it's an independent
    // editor window). If the child exits before us it becomes a zombie
    // until we exit and init reaps it — harmless for a GUI editor.
    return 1;
}

int spawnNewInstanceWithFile(const char *file) {
    char path[4096];
    if (!getSelfPath(path, sizeof(path))) {
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return 0;
    }
    if (pid == 0) {
        // Child: replace with a copy of this program opening `file`
        // (main() treats argv[1] as a file to load).
        execl(path, path, file, (char *)NULL);
        _exit(127);
    }

    return 1;
}

// TODO: compile/run on linux + mac. windows only for now
int spawnTool(const char *cmdline) { (void)cmdline; return 0; }
int readTool(char *buf, int size) { (void)buf; (void)size; return 0; }
int toolDone(int *exitCode) { *exitCode = -1; return 1; }
void closeTool(void) {}
void killTool(void) {}
int spawnConsole(const char *cmdline, const char *dir) { (void)cmdline; (void)dir; return 0; }
void killConsole(void) {}
int spawnServer(const char *cmdline) { (void)cmdline; return 0; }
int writeServer(const char *buf, int len) { (void)buf; (void)len; return 0; }
int readServer(char *buf, int size) { (void)buf; (void)size; return 0; }
int serverAlive(void) { return 0; }
void killServer(void) {}

#endif /* _WIN32 */
