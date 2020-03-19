cmake_minimum_required(VERSION 3.15)
project(pathlib)

set(SOURCE_LIB pathlib.c)

add_library(pathlib STATIC ${SOURCE_LIB})