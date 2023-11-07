file(GLOB_RECURSE src
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(stage ${src})

add_definitions(-DASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
add_definitions(-DSHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/")
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE=1)

# Custom command to compile shaders before building
file(GLOB_RECURSE shader_src
  ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/*.vert
  ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/*.comp
  ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/*.frag
)

add_custom_target(
  compile_shaders 
  COMMAND ${CMAKE_CURRENT_LIST_DIR}/assets/compile_shaders.bat ${CMAKE_CURRENT_LIST_DIR}/assets/shaders ${CMAKE_CURRENT_LIST_DIR}/assets
  DEPENDS ${shader_src}
)

add_dependencies(stage compile_shaders)

if (WIN32)
  add_definitions(-DGLEW_STATIC)
  target_link_libraries(stage glfw ${Vulkan_LIBRARIES} glm libnoise Bitsery::bitsery)
  target_include_directories(stage PRIVATE ${Vulkan_INCLUDE_DIRS})
else()
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(stage glfw glm ${Vulkan_LIBRARIES} Threads::Threads)
endif()