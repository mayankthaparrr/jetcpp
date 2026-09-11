cmake_minimum_required(VERSION 4.3)

project(ATR C)

set(CMAKE_C_STANDARD 11)

include(FetchContent)

FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
)

FetchContent_MakeAvailable(raylib)

add_executable(ATR
        main.c
        ui/ui_ribbon.c
        ui/ui_ribbon.h
        ui/ui_textarea.c
        ui/ui_textarea.h
        ui/ui_ribbonEvents.c
        ui/ui_ribbonEvents.h
)

target_link_libraries(ATR PRIVATE raylib)