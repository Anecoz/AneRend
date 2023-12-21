#pragma once

#include "../../util/Uuid.h"

#include <string>
#include <glm/glm.hpp>

namespace render::asset {

struct Prefab
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

  util::Uuid _model;
  util::Uuid _skeleton;
  std::vector<util::Uuid> _materials;

  glm::mat4 _transform = glm::mat4(1.0f);
  glm::vec3 _tint = glm::vec3(0.0f);
  glm::vec4 _boundingSphere = glm::vec4(0.0f, 0.0f, 0.0f, 30.0f); // xyz sphere center, w radius

  // Child prefabs, if this prefab gets instantiated, all children will be instantiated as renderable children.
  util::Uuid _parent;
  std::vector<util::Uuid> _children;
};

}