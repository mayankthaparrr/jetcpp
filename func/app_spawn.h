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

#endif // APP_SPAWN_H
