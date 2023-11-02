#pragma once

#include "Mesh.h"
#include "../Identifiers.h"

#include <vector>

namespace render::asset {

struct Model
{
  render::ModelId _id = INVALID_ID;
  std::vector<Mesh> _meshes;
};

}

