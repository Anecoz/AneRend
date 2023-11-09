file(GLOB_RECURSE src
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(anedit ${src})

add_definitions(-DASSET_PATH="${CMAKE_SOURCE_DIR}/assets/")
add_definitions(-DSHADER_PATH="${CMAKE_SOURCE_DIR}/assets/shaders/")
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE=1)

# Custom command to compile shaders before building
file(GLOB_RECURSE shader_src
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.vert
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.comp
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.frag
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.rgen
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.rchit
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.rmiss
)

add_custom_target(
  compile_shaders 
  COMMAND ${CMAKE_SOURCE_DIR}/assets/compile_shaders.bat ${CMAKE_SOURCE_DIR}/assets/shaders ${CMAKE_SOURCE_DIR}/assets
  DEPENDS ${shader_src}
)

add_dependencies(anedit compile_shaders)

if (WIN32)
  target_link_libraries(anedit anerend glfw glm imgui tinygltf lodepng)
    target_include_directories(anedit PRIVATE ${Vulkan_INCLUDE_DIRS})
else()
endif()