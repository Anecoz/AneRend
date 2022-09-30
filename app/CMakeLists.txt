file(GLOB_RECURSE src
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(stage ${src})

add_definitions(-DASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
add_definitions(-DSHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/")

if (WIN32)
  add_definitions(-DGLEW_STATIC)
  target_link_libraries(stage glfw ${Vulkan_LIBRARIES} glm)
  target_include_directories(stage PRIVATE ${Vulkan_INCLUDE_DIRS})
else()
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(stage glfw glm ${Vulkan_LIBRARIES} Threads::Threads)
endif()