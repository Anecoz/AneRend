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

  # NativeFileDialog_extended (nfd)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nativefiledialog-extended)

  # libktx
  set(KTX_FEATURE_STATIC_LIBRARY ON)
  set(KTX_FEATURE_TOOLS OFF CACHE BOOL "Create KTX tools" FORCE)
  set(KTX_FEATURE_TESTS OFF CACHE BOOL "Create KTX unit tests" FORCE)
  set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "Enable OpenGL texture upload" FORCE)
  set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "Enable Vulkan texture upload" FORCE)
  set(KTX_FEATURE_LOADTEST_APPS "" CACHE STRING  "Load test apps test the upload feature by displaying various KTX textures. Select which to create. \"OpenGL\" includes OpenGL ES." FORCE)
  add_definitions(-D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
  add_compile_options(/Wv:18)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/KTX-Software)
  #target_compile_definitions(ktx PUBLIC /Wv:18)

  # MikkTSpace
  add_library(mikktspace STATIC 
    ${CMAKE_CURRENT_LIST_DIR}/MikkTSpace/mikktspace.c
    ${CMAKE_CURRENT_LIST_DIR}/MikkTSpace/mikktspace.h)

  target_include_directories(mikktspace INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}/MikkTSpace/)

  # entt
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/entt)

else()  
endif()