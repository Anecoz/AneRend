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
  find_package(Vulkan REQUIRED)

  #imgui
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/imgui)

  #tinygltf
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tinygltf)
  
  #lodepng
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/LodePng)

  #bitsery
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bitsery)

  #stduuid
  set(UUID_SYSTEM_GENERATOR ON CACHE BOOL "Enable operating system uuid generator" FORCE)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/stduuid)

else()  
endif()