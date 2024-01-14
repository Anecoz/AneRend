#pragma once

#include "../../component/Components.h"

namespace render::internal {

struct InternalLight
{
  util::Uuid _id;
  component::Light _lightComp;
  glm::vec3 _pos;
};

}