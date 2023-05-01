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
  glm::vec4 _tint;
  uint32_t _firstMeshId;
  uint32_t _numMeshIds;
  uint32_t _visible;
};

struct GPUMeshMaterialInfo {
  int32_t _metallicTexIndex;
  int32_t _roughnessTexIndex;
  int32_t _normalTexIndex;
  int32_t _albedoTexIndex;
  int32_t _metallicRoughnessTexIndex;
};

struct GPUTranslationId
{
  uint32_t _renderableId;
  uint32_t _meshId;
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

struct GPULight {
  glm::vec4 _worldPos; // xyz is pos and w is range
  glm::vec4 _color; // w is enabled or not
};

struct GPUViewCluster {
  glm::vec4 _min; // view space
  glm::vec4 _max; // view space
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
  glm::mat4 invProj;
  glm::mat4 invViewProj;
  glm::mat4 directionalShadowMatrixProj;
  glm::mat4 directionalShadowMatrixView;
  glm::mat4 shadowMatrix[24];
  glm::vec4 cameraPos;
  glm::vec4 lightDir;
  glm::vec4 viewVector;
  float time;
  uint32_t screenWidth;
  uint32_t screenHeight;
};

struct GPUSSAOSampleUbo {
  glm::vec4 samples[64];
};

struct HiZPushConstants {
  uint32_t inputIdx;
  uint32_t outputIdx;
  uint32_t inputWidth;
  uint32_t inputHeight;
  uint32_t outputSize;
};

}
}