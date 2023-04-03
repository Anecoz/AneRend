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
// Spec part about alignments: https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#interfaces-resources-layout

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

struct GPUNonIndexDrawCallCmd {
  VkDrawIndirectCommand _command;
};

struct GPUGrassBlade {
  glm::vec4 _worldPos;
  glm::vec4 _cpData0;
  glm::vec4 _cpData1;
  glm::vec4 _cpData2;
  glm::vec4 _widthDir;
};

// Minimum supported size of push constants is 128 bytes! (RTX 3080 has a limit of 256 bytes)
struct GPUCullPushConstants {
  glm::mat4 _view;             // 4 * 4 * 4 = 64 bytes
  glm::vec4 _frustumPlanes[4]; // 4 * 4 * 4 = 64 bytes
  float _nearDist;             // 4 bytes
  float _farDist;              // 4 bytes
  uint32_t _drawCount;         // 4 bytes
  float _windDirX;          // 8 bytes
  float _windDirY;
                               // Total: 148 bytes
};

// This is used as a UBO and has follows std140 rules (use 4-byte things basically...)
struct GPUSceneData {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 directionalShadowMatrixProj;
  glm::mat4 directionalShadowMatrixView;
  glm::mat4 shadowMatrix[24];
  glm::vec4 cameraPos;
  glm::vec4 lightDir;
  glm::vec4 lightPos[4];
  glm::vec4 lightColor[4];
  glm::vec4 viewVector;
  float time;
};

}
}