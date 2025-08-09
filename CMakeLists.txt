cmake_minimum_required(VERSION 3.16)
project(App LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(raylib REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(App ${SOURCES})

include_directories(App "src/" "include/")

target_link_libraries(App PRIVATE raylib)

target_compile_options(App PRIVATE -Wall -Wextra -O2 -g)
