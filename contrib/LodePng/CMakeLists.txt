file(GLOB_RECURSE src
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(lodepng STATIC ${src})
target_include_directories(lodepng INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})