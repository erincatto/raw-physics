cmake_minimum_required(VERSION 3.23)
include(FetchContent)

message(STATUS "cmake source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "library postfix: ${CMAKE_DEBUG_POSTFIX}")

project(raw_physics LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)
