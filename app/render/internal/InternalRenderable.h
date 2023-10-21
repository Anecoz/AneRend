#pragma once

#include "../asset/Renderable.h"

namespace render::internal {

struct InternalRenderable
{
  render::asset::Renderable _renderable;

  // Index into the renderable material index buffer
  std::uint32_t _materialIndexBufferIndex = 0;

  // Offset into the GPU skeleton buffer
  std::uint32_t _skeletonOffset = 0;
};

}