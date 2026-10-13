# Jet C++

A Turbo C++ 3.0-style retro IDE written in C11 with raylib 5.5.

## Overview

- **What it is**: a single-file C/C++ editor with a Turbo-style menu ribbon, compile / run / debug built in, and syntax highlighting
- **Toolchain**: [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) (clang, lld, lldb, clangd) ships in `tools/llvm-mingw`
- **Platform**: Windows. The editor itself builds on POSIX, but compile / run / debug / clangd are Windows-only for now
- **Author**: Mayank Thapar

## Build

Built on Windows using CLion.

## Project Structure

```
main.c                   window, font, frame loop
func/app_spawn.c/.h      new windows, pipes for clang + clangd, console windows for Run / Debug
func/toolchain.c/.h      Run menu: compile with clang, run, debug with lldb, stop
func/lsp.c/.h            clangd client (Language Server Protocol over pipes, uses cJSON)
func/undo.c/.h           undo/redo history
ui/ui_ribbon.c/.h        menu ribbon (tabs, dropdowns, actions)
ui/ui_ribbonEvents.c/.h  File menu: new / open / save
ui/ui_textarea.c/.h      EDITOR CORE (lines, cursor, selection, drawing)
ui/ui_scroll.c/.h        scrollbars, wheel, zoom
ui/ui_messages.c/.h      Message pane (compiler output, click to jump)
ui/ui_syntax.c/.h        syntax highlighting: scanner + colours
third_party/cJSON/       JSON library for the clangd client
tools/llvm-mingw/        bundled toolchain
tests/test1/             test harnesses + run.sh
```

## Roadmap

1. ✅ Menu cut + wire Quit / Select All
2. ✅ Undo / Redo
3. ✅ Compile / Run / Debug (lldb window) / Stop + Message pane with click-to-jump
4. ✅ Syntax highlighting (Turbo palette + clangd)
5. Unsaved-changes dialog (Save / Don't Save / Cancel before Quit / Open)
6. Search: Find / Replace / Go To Line
7. clangd: autocomplete, live errors, go to definition
8. Project: multi-file builds
9. Debugger inside the IDE (lldb-dap)

## Credits

- [raylib 5.5](https://www.raylib.com/): zlib license
- [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended): zlib license (master branch, new API)
- [cJSON v1.7.19](https://github.com/DaveGamble/cJSON): MIT license (`third_party/cJSON/LICENSE`)
- [llvm-mingw](https://github.com/mstorsjo/llvm-mingw): LLVM is Apache 2.0 with LLVM Exceptions, mingw-w64 has its own licenses (`tools/llvm-mingw/LICENSE.TXT` and the `COPYING` files under `tools/llvm-mingw`)
- [Px437 IBM VGA 9x16](https://int10h.org/oldschool-pc-fonts/) from The Ultimate Oldschool PC Font Pack by VileR: CC BY-SA 4.0
- [Roboto font](https://fonts.google.com/specimen/Roboto): Apache License 2.0

## License

MIT
