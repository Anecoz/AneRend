#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <glm/glm.hpp>

#include <array>

namespace render {

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
};

}