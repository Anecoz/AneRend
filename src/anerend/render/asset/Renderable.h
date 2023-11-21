#pragma once 

#include "../../util/Uuid.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace render::asset {

struct Renderable
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name; // for debugging

  util::Uuid _model;
  util::Uuid _skeleton;
  std::vector<util::Uuid> _materials; // one for each mesh in the model (could be same ID for multiple meshes tho)

  glm::mat4 _transform;
  glm::vec3 _tint;
  glm::vec4 _boundingSphere; // xyz sphere center, w radius
  bool _visible = true;
};

}