file(GLOB_RECURSE src
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

file(GLOB_RECURSE common_src
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/*.cpp
)

file(GLOB_RECURSE headers
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_definitions(-DASSET_PATH="${CMAKE_SOURCE_DIR}/assets/")
add_definitions(-DSHADER_PATH="${CMAKE_SOURCE_DIR}/assets/shaders/")
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE=1)

add_library(anerend STATIC ${src} ${common_src})

if (WIN32)
  target_link_libraries(anerend glfw ${Vulkan_LIBRARIES} imgui lodepng glm libnoise Bitsery::bitsery stduuid ktx)
  target_include_directories(anerend PRIVATE ${Vulkan_INCLUDE_DIRS})

  #target_include_directories(anerend INTERFACE headers)
  target_include_directories(anerend INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
else()
endif()