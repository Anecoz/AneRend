#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

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
  uint32_t _modelOffset;
  uint32_t _numMeshes;
  uint32_t _skeletonOffset;
  uint32_t _visible;
  uint32_t _firstMaterialIndex;
  uint32_t _dynamicModelOffset;
  int32_t _terrainOffset = -1;
};

// See render::asset::Material
struct GPUMaterialInfo {
  glm::vec4 _baseColFac; // w unused
  glm::vec4 _emissive;
  glm::ivec4 _bindlessIndices; // metRough, albedo, normal, emissive
  glm::vec4 _metRough; // r metallic, g roughness
  //float _metallicFactor;
  //float _roughnessFactor;
};

struct GPUMeshInfo {
  glm::vec4 _minPos;
  glm::vec4 _maxPos;
  uint32_t _vertexOffset;
  uint32_t _indexOffset;
  uint64_t _blasRef;
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

struct GPUTileInfo {
  int _ddgiAtlasTex;
};

struct GPUTerrainInfo {
  glm::ivec4 _baseMaterials;
  glm::ivec4 _indices; // blendmap, heightmap, vegmap
  glm::ivec4 _tileInfo; // tilex, tiley
  glm::vec4 _extraData; // mpp
};

struct GPUPointLightShadowCube
{
  glm::mat4 _shadowMatrices[6];
};

struct GPUViewCluster {
  glm::vec4 _min; // view space
  glm::vec4 _max; // view space
};

// Minimum supported size of push constants is 128 bytes! (RTX 3080 has a limit of 256 bytes)
struct GPUCullPushConstants {
  glm::mat4 _view;             // 4 * 4 * 4 = 64 bytes
  glm::vec4 _frustumPlanes[4]; // 4 * 4 * 4 = 64 bytes
  glm::ivec4 _pointLightShadowInds; // 4 * 4 = 16 bytes
  float _nearDist;             // 4 bytes
  float _farDist;              // 4 bytes
  uint32_t _drawCount;         // 4 bytes
  float _windDirX;             // 4 bytes
  float _windDirY;             // 4 bytes
                               // Total: 164 bytes
};

enum SceneUboFlags : std::uint32_t
{
  UBO_SSAO_FLAG = 1,
  UBO_FXAA_FLAG = 1 << 1,
  UBO_DIRECTIONAL_SHADOWS_FLAG = 1 << 2,
  UBO_RT_SHADOWS_FLAG = 1 << 3,
  UBO_DDGI_FLAG = 1 << 4,
  UBO_DDGI_MULTI_FLAG = 1 << 5,
  UBO_SPECULAR_GI_FLAG = 1 << 6,
  UBO_SS_PROBES_FLAG = 1 << 7,
  UBO_BS_VIS_FLAG = 1 << 8,
  UBO_HACK_FLAG = 1 << 9,
  UBO_RT_ON_FLAG = 1 << 10,
  UBO_BAKE_MODE_ON_FLAG = 1 << 11
};

// This is used as a UBO and has follows std140 rules (use 4-byte things basically...)
struct GPUSceneData {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 invView;
  glm::mat4 invProj;
  glm::mat4 invViewProj;
  glm::mat4 directionalShadowMatrixProj;
  glm::mat4 directionalShadowMatrixView;
  glm::vec4 cameraPos;
  glm::ivec4 cameraGridPos;
  glm::vec4 lightDir;
  glm::ivec4 bakeTileInfo; // x, y: tile idx, z: tile size
  glm::vec4 skyColor;
  float time;
  float delta;
  uint32_t screenWidth;
  uint32_t screenHeight;
  float sunIntensity;
  float skyIntensity;
  float exposure;
  uint32_t flags; // SceneUboFlags
  float bloomThresh;
  float bloomKnee;
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

struct GPUIrradianceProbe
{
  glm::vec4 pos; //xyz and w is active/inactive
  glm::vec4 irr0; //n.r, n.g, n.b, sum-of-weights
  glm::vec4 irr1; //e.r, e.g, e.b, sum-of-weights
  glm::vec4 irr2; //s.r, s.g, s.b, sum-of-weights
  glm::vec4 irr3; //w.r, w.g, w.b, sum-of-weights
  glm::vec4 irr4; //u.r, u.g, u.b, sum-of-weights
  glm::vec4 irr5; //d.r, d.g, d.b, sum-of-weights
};

struct alignas(16) GPUParticle
{
  glm::vec4 initialVelocity; // w is scale
  glm::vec4 initialPosition; // w is lifetime
  glm::vec4 currentPosition; // w is spawnDelay
  glm::vec4 currentVelocity; // w is elapsedTime
  int alive;
  uint32_t renderableId;
};

// Maps the internal ids (such as RenderableId) to an index in the GPU buffers
// e.g. Renderable rend = renderableBuffer[idMapBuffer[renderableId]];
struct GPUIdMap
{
  uint32_t _meshIndex;
  uint32_t _renderableIndex;
  uint32_t _materialIndex;
};

}
}