#pragma once

#include "../Identifiers.h"
#include "BufferMemoryInterface.h"

namespace render::internal {

struct InternalMesh
{
  render::MeshId _id;

  uint32_t _numIndices;
  uint32_t _numVertices;

  // Offsets into fat buffers.
  BufferMemoryInterface::Handle _vertexHandle;
  BufferMemoryInterface::Handle _indexHandle;
  int64_t _indexOffset;
  uint32_t _vertexOffset;

  // For ray-tracing
  uint64_t _blasRef = 0;
};

}