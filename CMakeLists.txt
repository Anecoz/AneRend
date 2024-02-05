cmake_minimum_required(VERSION 3.7)
project(stage)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

include(contrib/contrib.cmake)
add_subdirectory(src)