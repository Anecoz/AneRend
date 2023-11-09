#pragma once

#include "../asset/Renderable.h"

namespace render::internal {

struct InternalRenderable
{
  render::asset::Renderable _renderable;

  // Convenience
  std::vector<render::MeshId> _meshes;

  // Index into the renderable material index buffer
  std::uint32_t _materialIndexBufferIndex = 0;

  // Offset into the GPU skeleton buffer
  std::uint32_t _skeletonOffset = 0;

  // Only used for ray tracing: Offset where to write animated vertices
  MeshId _rtFirstDynamicMeshId = INVALID_ID;
};

}