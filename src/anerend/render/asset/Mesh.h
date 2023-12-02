#pragma once

#include "../Vertex.h"
#include "../../util/Uuid.h"

#include <glm/glm.hpp>
#include <vector>

namespace render::asset {

struct Mesh
{
  util::Uuid _id = util::Uuid::generate();
  std::vector<render::Vertex> _vertices;
  std::vector<std::uint32_t> _indices;

  // These are in model space, i.e. need to be multiplied by a model transform
  glm::vec3 _minPos;
  glm::vec3 _maxPos;
};

}