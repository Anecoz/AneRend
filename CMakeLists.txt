cmake_minimum_required(VERSION 3.7)
project(stage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(contrib/contrib.cmake)
add_subdirectory(app)