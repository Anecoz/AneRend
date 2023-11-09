#pragma once

#include "../Identifiers.h"

#include <vector>

namespace render::internal
{

struct InternalModel
{
  render::ModelId _id;
  std::vector<render::MeshId> _meshes;
};

}