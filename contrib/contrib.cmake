if (WIN32)
  #glm
  add_library(glm INTERFACE)
  target_include_directories(glm INTERFACE ${CMAKE_CURRENT_LIST_DIR}/glm/include)

  #glfw3
  add_library(glfw STATIC IMPORTED)
  set_target_properties(glfw PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/glfw3/lib/glfw3.lib)
  target_include_directories(glfw INTERFACE ${CMAKE_CURRENT_LIST_DIR}/glfw3/include)

  #libnoise
  add_library(libnoise STATIC IMPORTED)
  set_target_properties(libnoise PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libnoise/lib/Release/LibNoise64.lib)
  target_include_directories(libnoise INTERFACE ${CMAKE_CURRENT_LIST_DIR}/libnoise/include)

  #vulkan
  set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.239.0")
  find_package(Vulkan REQUIRED)

else()  
  find_package(glfw3 REQUIRED)
  find_package(glm REQUIRED)
  find_package(vulkan REQUIRED)
endif()