#pragma once

#include "../../util/Uuid.h"

#include <string>

namespace render::asset {

struct Prefab
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

  util::Uuid _model;
  util::Uuid _skeleton;
  std::vector<util::Uuid> _materials;
};

}