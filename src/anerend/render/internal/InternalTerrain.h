#pragma once

#include "../../util/Uuid.h"

#include <glm/glm.hpp>

#include <array>

namespace render::internal {

struct InternalTerrain
{
  util::Uuid _id;

  util::Uuid _heightmap;

  // 4 base materials to blend between. These are ids of materials
  std::array<util::Uuid, 4> _baseMaterials;

  // Texture id of RGBA8 blend mask used to blend between base materials
  util::Uuid _blendMask;
};

}