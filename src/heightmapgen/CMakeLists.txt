file(GLOB_RECURSE src
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)


file(GLOB_RECURSE common_src
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/util/noiseutils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/util/noiseutils.cpp
)

add_executable(heightmapgen ${src} ${common_src})

if (WIN32)
  target_link_libraries(heightmapgen libnoise lodepng)
else()
endif()