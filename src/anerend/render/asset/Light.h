#pragma once

#include "../../util/Uuid.h"

#include <glm/glm.hpp>

namespace render::asset {

struct Light
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

  glm::vec3 _pos = glm::vec3(0.0f);
  glm::vec3 _color = glm::vec3(1.0f, 0.0f, 0.0f);
  float _range = 2.0f;
  bool _enabled = true;
  // TODO:
  // Type, i.e. directional, point, spot
};

}