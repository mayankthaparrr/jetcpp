# Jet C++

A Turbo C++ 3.0-style retro IDE written in C11 with raylib 5.5.

## Overview

- **Target**: Single-document text editor with menu ribbon
- **Long-term goal**: MinGW + LLVM toolchain integration (compile, run, debug)
- **Platform**: Windows (primary), POSIX compatible
- **Author**: Mayank Thapar

## Quick Start

### Prerequisites
- CLion 2026.2.1 (bundled MinGW + Ninja)
- CMake 4.3+

### Build
```powershell
$env:PATH = "C:\Program Files\JetBrains\CLion 2026.2.1\bin\mingw\bin;$env:PATH"
& "C:\Program Files\JetBrains\CLion 2026.2.1\bin\ninja\win\x64\ninja.exe" -C cmake-build-debug
```

### Run
```powershell
.\cmake-build-debug\JetCpp.exe
```

## Current Features

### Working
- File: New (spawns new window), Open, Save, Save As, Quit
- Edit: Undo/Redo (Ctrl+Z, Ctrl+Y / Ctrl+Shift+Z), Select All, Copy/Cut/Paste (Ctrl+C/X/V), Ctrl+A
- Undo model: linear in-memory delta stack (100-step cap), typing coalesces into one step per line-run, redo pruned on new edit
- Navigation: Arrow keys, Home/End, Ctrl+Left/Right (word), Ctrl+Up/Down (top/bottom)
- Scrolling: Mouse wheel (vertical), Shift+wheel (horizontal), Ctrl+wheel (zoom)
- Zoom: Ctrl++/= (in), Ctrl+- (out), Ctrl+0 (reset) — editor only, ribbon fixed

### Stubs (highlight only)
- Find/Replace, Go To Line
- Run: Compile, Run, Arguments, Stop
- Project, Options, Help → About
- Edit: Cut/Paste/Clear (keyboard shortcuts work; menu items unwired)

## Project Structure

```
main.c                 — window, fonts, frame loop
func/app_spawn.c/.h    — cross-platform new-window spawning
func/undo.c/.h         — linear undo/redo history (delta stack, 100-step cap)
ui/ui_ribbon.c/.h      — menu ribbon (7 tabs, dropdowns, actions)
ui/ui_ribbonEvents.c/.h — File menu implementations
ui/ui_textarea.c/.h    — EDITOR CORE (lines, cursor, selection, rendering)
ui/ui_scroll.c/.h      — scrollbars, wheel handling, zoom
```

## Roadmap

1. ✅ Menu cut + wire Quit/Select All
2. ✅ Undo/Redo — delta stack (func/undo.c), coalesced typing runs, Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z
3. **Toolchain v1**: `toolchain.c` with `runTool()`, Options→Directories config, Compile menu
4. **Messages pane**: compile output area (stub exists in ui_textarea.c)
5. **Run**: compile → execute (reuse func/app_spawn.c pattern)
6. **Project**: multi-file support
7. **Debugger**: LLDB integration

## Credits

- [raylib 5.5](https://www.raylib.com/) — zlib license
- [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended) — MIT license (master branch, new API)
- [Px437 IBM VGA 9x16 font](https://int10h.org/oldschool-pc-fonts/) — public domain / CC0
- [Roboto font](https://fonts.google.com/specimen/Roboto) — Apache License 2.0

## License

MIT