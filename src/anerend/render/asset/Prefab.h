#pragma once

#include "../../util/Uuid.h"

namespace render::asset {

struct Prefab
{
  util::Uuid _id = util::Uuid::generate();

  util::Uuid _model;
  util::Uuid _skeleton;
  std::vector<util::Uuid> _materials;
};

}