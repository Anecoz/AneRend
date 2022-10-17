#pragma once

#include <glm/glm.hpp>

namespace render {

struct StandardUBO {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 shadowMatrix[24];
  glm::vec4 cameraPos;
  glm::vec4 lightDir;
  glm::vec4 lightPos[4];
  glm::vec4 lightColor[4];
};

}