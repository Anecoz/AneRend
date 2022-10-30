#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "MeshId.h"

namespace render {
namespace gpu {

// Alignment is super annoying... std430 for these things
// If the alignment is wrong, the validation layers _sometimes_ tell you.
// Other times you just get extremely weird behaviour since the strides between
// shader invocations etc. will be off/wrong.

struct alignas(16) GPURenderable {
  glm::mat4 _transform;
  glm::vec4 _bounds;
  uint32_t _meshId; // Also index into the mesh GPU buffer
  uint32_t _visible; // TODO: Not sure about if there is a bool type...
};

// One per mesh
struct GPUDrawCallCmd {
  VkDrawIndexedIndirectCommand _command;
};

// Minimum supported size of push constants is 128 bytes! (RTX 3080 has a limit of 256 bytes)
struct GPUCullPushConstants {
  glm::mat4 _view;             // 4 * 4 * 4 = 64 bytes
  glm::vec4 _frustumPlanes[4]; // 4 * 4 * 4 = 64 bytes
  uint32_t _drawCount;         // 4 bytes
                               // Total: 132 bytes
};

}
}