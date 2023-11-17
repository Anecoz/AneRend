#pragma once

#include "../util/Uuid.h"
#include <glm/glm.hpp>

namespace render {

struct Particle
{
  glm::vec3 _initialVelocity;
  glm::vec3 _initialPosition;
  float _scale;
  float _lifetime;
  float _spawnDelay;
  util::Uuid _renderableId;
};

}