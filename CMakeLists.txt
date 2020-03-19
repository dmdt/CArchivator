cmake_minimum_required(VERSION 3.15)
project(OS_Archivator C)

set(CMAKE_C_STANDARD 11)

include_directories(lib/pathlib)
add_subdirectory(lib/pathlib)

add_executable(OS_Archivator main.c)

target_link_libraries(OS_Archivator pathlib)
