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
        app_spawn/app_spawn.c
        app_spawn/app_spawn.h
        ui/ui_ribbon.c
        ui/ui_ribbon.h
        ui/ui_textarea.c
        ui/ui_textarea.h
        ui/ui_ribbonEvents.c
        ui/ui_ribbonEvents.h
        ui/ui_scroll.c
        ui/ui_scroll.h
)

set_target_properties(JetCpp PROPERTIES
        OUTPUT_NAME "Jet C++"
)

target_include_directories(JetCpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(JetCpp PRIVATE
        raylib
        nfd
)

# Editor fonts ship next to the executable; main.c resolves them relative
# to the exe via GetApplicationDirectory() (fixes hardcoded C:/DEV/ATR paths).
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Px437_IBM_VGA_9x16.ttf"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")