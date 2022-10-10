#pragma once

#include <glm/glm.hpp>

namespace render {

struct ShadowUBO {
  glm::mat4 shadowMatrix;
};

}