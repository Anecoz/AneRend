#pragma once

#include "../../util/Uuid.h"

#include <glm/glm.hpp>

namespace render::debug {

struct Geometry
{
  util::Uuid _meshId;
  bool _wireframe;
  glm::vec3 _tint;
  glm::mat4 _modelToWorld;
};

}