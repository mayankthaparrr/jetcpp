//
// app_spawn.h — launching new instances of this program.
//
// Created for ATR / Jet C++ editor.
//
#ifndef APP_SPAWN_H
#define APP_SPAWN_H

// Launches a new instance of this program. The new process starts up with
// the same default state as a fresh launch (an empty, unsaved document),
// which is exactly the "New window" behavior we want.
//
// Returns 1 on success, 0 on failure (caller is expected to fall back to
// something sensible, e.g. clearing the current document).
//
// Supported platforms:
//   - Windows: CreateProcess
//   - Linux:   fork + exec, executable path from /proc/self/exe
//   - macOS:   fork + exec, executable path from _NSGetExecutablePath
//
// NOT cross-platform beyond the above: there is no standard C (or POSIX)
// way to get the current executable's path, so other platforms must add
// their own case in getSelfPath() (e.g. sysctl KERN_PROC_PATHNAME on BSD).
int spawnNewInstance(void);

// Like spawnNewInstance(), but the new window opens with `file` loaded
// (e.g. Help -> About opening the project README in its own window).
// Returns 1 on success, 0 on failure.
int spawnNewInstanceWithFile(const char *file);

// runs a tool (clang) in the background, its output going into a pipe.
// cmdline is the whole command line with paths already in quotes. 1 = started
int spawnTool(const char *cmdline);
// copies whatever output is waiting, never waits for more. returns bytes copied
int readTool(char *buf, int size);
// 1 once the tool has exited (sets exitCode). read the rest, then closeTool()
int toolDone(int *exitCode);
void closeTool(void);
void killTool(void);

// opens a new console window running cmdline in dir (the program, or lldb)
int spawnConsole(const char *cmdline, const char *dir);
// closes that window and everything it started
void killConsole(void);

// a server (clangd) that keeps running, with a pipe each way. it dies with Jet C++
int spawnServer(const char *cmdline);
int writeServer(const char *buf, int len); // 1 = all of it written
int readServer(char *buf, int size);       // like readTool: never waits
int serverAlive(void);
void killServer(void);

#endif // APP_SPAWN_H
