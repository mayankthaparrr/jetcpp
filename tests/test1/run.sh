#!/usr/bin/env bash
# builds + runs the harnesses with the repo's llvm-mingw clang, with
# AddressSanitizer + UBSan on so memory bugs crash loudly instead of hiding.
#   editor_harness:    typing, selection, undo, utf-8, tabs, scrolling, save/load
#   toolchain_harness: Run -> Compile/Run/Debug/Stop + the Message pane, with real
#                      clang. it pops up a console window + lldb for a second or two.
#   lsp_harness:       func/lsp.c talking to the real clangd, prints what it says
# needs cmake-build-debug configured once (for the raylib/nfd headers).
# run from git bash:  tests/test1/run.sh
set -e
cd "$(dirname "$0")"
R="$(cd ../.. && pwd)"
RW="$(cygpath -m "$R")" # C:/DEV/ATR style, for the C code
export PATH="$R/tools/llvm-mingw/bin:$PATH"
mkdir -p build

FLAGS=(-std=c11 -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined
       -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
       "-DJET_SOURCE_DIR=\"$RW\""
       -I"$R" -I. -I"$R/third_party/cJSON" -I"$R/cmake-build-debug/_deps/raylib-src/src" -I"$R/cmake-build-debug/_deps/nfde-src/src/include")

clang "${FLAGS[@]}" editor_harness.c \
    "$R/ui/ui_textarea.c" "$R/ui/ui_scroll.c" "$R/ui/ui_syntax.c" "$R/ui/ui_ribbonEvents.c" "$R/ui/ui_messages.c" "$R/func/undo.c" \
    -o build/editor_harness.exe

clang "${FLAGS[@]}" toolchain_harness.c \
    "$R/ui/ui_textarea.c" "$R/ui/ui_scroll.c" "$R/ui/ui_syntax.c" "$R/ui/ui_ribbonEvents.c" "$R/func/undo.c" "$R/func/app_spawn.c" \
    -o build/toolchain_harness.exe

clang "${FLAGS[@]}" lsp_harness.c "$R/func/lsp.c" "$R/third_party/cJSON/cJSON.c" "$R/func/toolchain.c" \
    "$R/ui/ui_textarea.c" "$R/ui/ui_scroll.c" "$R/ui/ui_syntax.c" "$R/ui/ui_ribbonEvents.c" "$R/ui/ui_messages.c" "$R/func/undo.c" "$R/func/app_spawn.c" \
    -o build/lsp_harness.exe

cd build
echo "== editor"
./editor_harness.exe
echo "== toolchain"
./toolchain_harness.exe "$(cygpath -w "$PWD/work")"
echo "== lsp"
./lsp_harness.exe "$(cygpath -w "$PWD/work")"
