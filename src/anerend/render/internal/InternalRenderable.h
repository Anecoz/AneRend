#pragma once

#include "../../component/Components.h"

namespace render::internal {

struct InternalRenderable
{
  component::Renderable _renderable;

  util::Uuid _id;

  // Convenience
  std::vector<util::Uuid> _meshes;

  glm::mat4 _globalTransform;
  glm::mat4 _invGlobalTransform;

  // Index into the renderable material index buffer
  std::uint32_t _materialIndexBufferIndex = 0;

  // Offset into the GPU skeleton buffer
  std::uint32_t _skeletonOffset = 0;

  // Index into the model buffer, to find meshes
  std::uint32_t _modelBufferOffset = 0;

  // Only used for ray tracing: Offset where to write animated vertices
  std::vector<util::Uuid> _dynamicMeshes;
  std::uint32_t _dynamicModelBufferOffset = 0;

  // Offset into terrain buffer on GPU
  bool _isTerrain = false;
};

}