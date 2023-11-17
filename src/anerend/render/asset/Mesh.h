#pragma once

#include "../Vertex.h"
#include "../../util/Uuid.h"

#include <vector>

namespace render::asset {

struct Mesh
{
  util::Uuid _id = util::Uuid::generate();
  std::vector<render::Vertex> _vertices;
  std::vector<std::uint32_t> _indices;
};

}