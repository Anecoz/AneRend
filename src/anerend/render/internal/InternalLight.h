#pragma once

#include "../../component/Components.h"

namespace render::internal {

struct InternalLight
{
  component::Light _lightComp;
  glm::vec3 _pos;
};

}