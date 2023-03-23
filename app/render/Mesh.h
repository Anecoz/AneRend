#pragma once

#include <cstddef>

#include "MeshId.h"

namespace render {

struct Mesh {
  MeshId _id;

  // Offsets into the fat mesh buffer
  std::size_t _vertexOffset;
  std::size_t _indexOffset;

  std::size_t _numVertices;
  std::size_t _numIndices;
};

}