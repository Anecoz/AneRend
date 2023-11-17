#pragma once

#include "Mesh.h"
#include "../../util/Uuid.h"

#include <vector>

namespace render::asset {

struct Model
{
  util::Uuid _id = util::Uuid::generate();
  std::vector<Mesh> _meshes;
};

}

