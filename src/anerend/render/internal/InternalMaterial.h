#pragma once

#include "../../util/Uuid.h"

#include <glm/glm.hpp>

namespace render::internal {

struct InternalMaterial
{
  util::Uuid _id;

  util::Uuid _albedoTex;
  util::Uuid _metRoughTex;
  util::Uuid _normalTex;
  util::Uuid _emissiveTex;

  glm::vec3 _baseColFactor;
  glm::vec4 _emissive;
  float _metallicFactor;
  float _roughnessFactor;
};

}