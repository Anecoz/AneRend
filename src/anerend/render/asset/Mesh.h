#pragma once

#include "../Vertex.h"
#include "../Identifiers.h"

#include <vector>

namespace render::asset {

struct Mesh
{
  render::MeshId _id = INVALID_ID; // Filled in once added to a scene.
  std::vector<render::Vertex> _vertices;
  std::vector<std::uint32_t> _indices;
};

}