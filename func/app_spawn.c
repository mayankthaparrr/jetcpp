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

#endif /* _WIN32 */
