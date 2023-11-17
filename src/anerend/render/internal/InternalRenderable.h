#pragma once

#include "../asset/Renderable.h"

namespace render::internal {

struct InternalRenderable
{
  render::asset::Renderable _renderable;

  // Convenience
  std::vector<util::Uuid> _meshes;

  // Index into the renderable material index buffer
  std::uint32_t _materialIndexBufferIndex = 0;

  // Offset into the GPU skeleton buffer
  std::uint32_t _skeletonOffset = 0;

  // Index into the model buffer, to find meshes
  std::uint32_t _modelBufferOffset = 0;

  // Only used for ray tracing: Offset where to write animated vertices
  std::vector<util::Uuid> _dynamicMeshes;
  std::uint32_t _dynamicModelBufferOffset = 0;
  //MeshId _rtFirstDynamicMeshId = INVALID_ID;
};

}