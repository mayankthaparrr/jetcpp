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
        ui_ribbon.c
        ui_ribbon.h
)

target_link_libraries(ATR PRIVATE raylib)