#pragma once

#include "../render/Vertex.h"

#include <vector>

namespace graphicsutil {

// Counter clockwise
// Vulkan NDC is
// x -> right
// y -> down
// z -> in toward screen
static std::vector<render::Vertex> createScreenQuad() {
  float depth = 0.1f; // Vulkan NDC depth range is 0,1, be close to camera...
  glm::vec3 color {1.0f, 1.0f, 1.0f}; // don't care
  glm::vec3 normal {0.0f, 0.0f, 0.0f}; // don't care
  return { 
    {{-1.0f, -1.0f, depth}, color, normal, {0.0f, 1.0f}},
    {{-1.0f, -0.5f, depth}, color, normal, {0.0f, 0.0f}},
    {{-0.5f, -1.0f, depth}, color, normal, {1.0f, 1.0f}},

    {{-0.5f, -1.0f, depth}, color, normal, {1.0f, 1.0f}},
    {{-1.0f, -0.5f, depth}, color, normal, {0.0f, 0.0f}},
    {{-0.5f, -0.5f, depth}, color, normal, {1.0f, 0.0f}}
  };
}

}