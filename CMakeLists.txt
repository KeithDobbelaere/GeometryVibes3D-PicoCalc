# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(GeometryVibes3D-PicoCalc C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(GeometryVibes3D-PicoCalc
    src/main.cpp
    src/app/App.cpp
    src/game/Game.cpp
    src/render/Project.cpp
    src/render/DrawList.cpp
    src/render/Renderer.cpp
    src/platform/pico/PicoPlatform.cpp
)

pico_set_program_name(GeometryVibes3D-PicoCalc "GeometryVibes3D-PicoCalc")
pico_set_program_version(GeometryVibes3D-PicoCalc "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(GeometryVibes3D-PicoCalc 0)
pico_enable_stdio_usb(GeometryVibes3D-PicoCalc 1)

# Add the standard library to the build
target_link_libraries(GeometryVibes3D-PicoCalc
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(GeometryVibes3D-PicoCalc PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(GeometryVibes3D-PicoCalc)

