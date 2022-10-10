#pragma once

#include <glm/glm.hpp>

namespace render {

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec2 uv;
};

}