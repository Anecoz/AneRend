#pragma once

#include "Mesh.h"
#include "../../util/Uuid.h"

#include <string>
#include <vector>

namespace render::asset {

struct Model
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;
  std::vector<Mesh> _meshes;
};

}

