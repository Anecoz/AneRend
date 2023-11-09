file(GLOB_RECURSE src
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(imgui STATIC ${src})
target_link_libraries(imgui PRIVATE glfw ${Vulkan_LIBRARIES})
target_include_directories(imgui PRIVATE ${Vulkan_INCLUDE_DIRS})

target_include_directories(imgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})