#pragma once

#include "../render/Vertex.h"

#include <cstdlib>
#include <vector>

namespace graphicsutil {

// Counter clockwise
// Vulkan NDC is
// x -> right
// y -> down
// z -> in toward screen
static std::vector<render::Vertex> createScreenQuad(float widthPercent, float heightPercent) {
  float depth = 0.1f; // Vulkan NDC depth range is 0,1, be close to camera...
  glm::vec3 color {1.0f, 1.0f, 1.0f}; // don't care
  glm::vec3 normal {0.0f, 0.0f, 0.0f}; // don't care

  float x = -1.0f + widthPercent * 2.0f;
  float y = -1.0f + heightPercent * 2.0f;

  return { 
    {{-1.0f, -1.0f, depth}, color, normal, {0.0f, 0.0f}},
    {{-1.0f, y, depth}, color, normal, {0.0f, 1.0f}},
    {{x, -1.0f, depth}, color, normal, {1.0f, 0.0f}},

    {{x, -1.0f, depth}, color, normal, {1.0f, 0.0f}},
    {{-1.0f, y, depth}, color, normal, {0.0f, 1.0f}},
    {{x, y, depth}, color, normal, {1.0f, 1.0f}}
  };
}

static glm::vec3 randomColor()
{
  return glm::vec3{
      static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 1.0f)),
      static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 1.0f)),
      static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 1.0f))
  };
}

static void createUnitCube(std::vector<render::Vertex>* vertOut, std::vector<std::uint32_t>* indOut, bool randomColor = true) {
  glm::vec3 color{ 1.0f };

  if (randomColor) {
    color = glm::vec3 {
      static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 1.0f)),
      static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 1.0f)),
      static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 1.0f))
    };
  }

  glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
  glm::vec2 tex{ 0.0f, 0.0f };

  std::vector<std::uint32_t> indices{
    //Top
    2, 7, 6,
    2, 3, 7,

    //Bottom
    0, 4, 5,
    0, 5, 1,

    //Left
    0, 2, 6,
    0, 6, 4,

    //Right
    1, 7, 3,
    1, 5, 7,

    //Front
    0, 3, 2,
    0, 1, 3,

    //Back
    4, 6, 7,
    4, 7, 5
  };

  // pos, color, normal, tex
  std::vector<render::Vertex> verts{
    {{-0.5, -0.5, 0.5}, color, normal, tex}, // 0
    {{0.5, -0.5, 0.5}, color, normal, tex},//1
    {{-0.5, 0.5, 0.5}, color, normal, tex},//2
    {{0.5, 0.5, 0.5}, color, normal, tex},//3
    {{-0.5, -0.5, -0.5}, color, normal, tex},//4
    {{0.5, -0.5, -0.5}, color, normal, tex},//5
    {{-0.5, 0.5, -0.5}, color, normal, tex},//6
    {{0.5, 0.5, -0.5}, color, normal, tex}//7
  };

  *vertOut = verts;
  *indOut = indices;
}

}