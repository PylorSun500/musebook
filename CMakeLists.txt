cmake_minimum_required(VERSION 3.28)

project(Musebook
    VERSION 0.1.0
    DESCRIPTION "A notebook application for music theory study"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# 警告集中在一个 INTERFACE target 上，由各 target 显式链接。
# 这样第三方依赖（阶段 1.5 的乐谱库）不会被本项目的警告级别波及。
add_library(musebook_warnings INTERFACE)

if(MSVC)
    target_compile_options(musebook_warnings INTERFACE
        /W4
        /permissive-
    )
else()
    target_compile_options(musebook_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
    )
endif()

add_subdirectory(src)
