#pragma once

#include <glm/glm.hpp>

namespace render {

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec4 tangent;
  glm::vec4 jointWeights;
  glm::vec2 uv;
  float padding = 0.0; // 20 floats (5 * vec4)
  glm::i16vec4 jointIds; // 8 bytes
  float padding1 = 0.0;
  float padding2 = 0.0;
};

}