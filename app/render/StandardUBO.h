#pragma once

#include <glm/glm.hpp>

namespace render {

struct StandardUBO {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 shadowMatrix;
  glm::vec4 cameraPos;
};

}