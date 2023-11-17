#pragma once

#include "../../util/Uuid.h"

#include <vector>

namespace render::internal
{

struct InternalModel
{
  util::Uuid _id;
  std::vector<util::Uuid> _meshes;
};

}