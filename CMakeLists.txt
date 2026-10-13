cmake_minimum_required(VERSION 4.3)

project("Jet C++" C)

set(CMAKE_C_STANDARD 11)

include(FetchContent)

FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
)

FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
        nfde
        GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
        GIT_TAG master
)

FetchContent_MakeAvailable(nfde)

add_executable(JetCpp
        main.c
        func/app_spawn.c
        func/app_spawn.h
        func/lsp.c
        func/lsp.h
        func/toolchain.c
        func/toolchain.h
        func/undo.c
        func/undo.h
        ui/ui_messages.c
        ui/ui_messages.h
        ui/ui_ribbon.c
        ui/ui_ribbon.h
        ui/ui_textarea.c
        ui/ui_textarea.h
        ui/ui_ribbonEvents.c
        ui/ui_ribbonEvents.h
        ui/ui_scroll.c
        ui/ui_scroll.h
        ui/ui_syntax.c
        ui/ui_syntax.h
        third_party/cJSON/cJSON.c
        third_party/cJSON/cJSON.h
)

set_target_properties(JetCpp PROPERTIES
        OUTPUT_NAME "Jet C++"
)

target_include_directories(JetCpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cJSON)

# toolchain.c falls back to tools/llvm-mingw in the repo when there's no
# toolchain folder next to the exe yet
target_compile_definitions(JetCpp PRIVATE JET_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(JetCpp PRIVATE
        raylib
        nfd
)

# Editor fonts ship next to the executable; main.c resolves them relative
# to the exe via GetApplicationDirectory() (fixes hardcoded C:/DEV/ATR paths).
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Px437_IBM_VGA_9x16.ttf"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
# Help -> About opens README.md from the exe dir. configure_file re-copies it
# whenever it changes (file(COPY) only ran on configure, so About went stale)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/README.md" "${CMAKE_CURRENT_BINARY_DIR}/README.md" COPYONLY)