#pragma once

#include <vulkan/vulkan.h>

#include "vma/vk_mem_alloc.h"

#include "asset/Model.h"
#include "../component/Components.h"
#include "asset/Material.h"
#include "asset/Texture.h"
#include "asset/TileInfo.h"
#include "animation/Animation.h"
#include "internal/InternalMesh.h"
#include "internal/InternalRenderable.h"
#include "internal/BufferMemoryInterface.h"
#include "internal/InternalLight.h"
#include "Vertex.h"
#include "GpuBuffers.h"
#include "RenderDebugOptions.h"
#include "RenderOptions.h"
#include "Camera.h"
#include "Particle.h"
#include "AccelerationStructure.h"

#include <unordered_map>

namespace render {

struct AssetUpdate
{
  explicit operator bool() const {
    return
      !_addedModels.empty() ||
      !_removedModels.empty() ||
      !_addedMaterials.empty() ||
      !_updatedMaterials.empty() ||
      !_removedMaterials.empty() ||
      !_addedTextures.empty() ||
      !_updatedTextures.empty() ||
      !_removedTextures.empty() ||
      !_addedAnimations.empty() ||
      !_removedAnimations.empty() ||
      !_addedRenderables.empty() ||
      !_updatedRenderables.empty() ||
      !_removedRenderables.empty() ||
      !_addedLights.empty() ||
      !_updatedLights.empty() ||
      !_removedLights.empty() ||
      !_addedTileInfos.empty() ||
      !_updatedTileInfos.empty() ||
      !_removedTileInfos.empty();
  }

  std::vector<asset::Model> _addedModels;
  std::vector<util::Uuid> _removedModels;

  std::vector<asset::Material> _addedMaterials;
  std::vector<asset::Material> _updatedMaterials;
  std::vector<util::Uuid> _removedMaterials;

  std::vector<asset::Texture> _addedTextures;
  std::vector<asset::Texture> _updatedTextures;
  std::vector<util::Uuid> _removedTextures;

  std::vector<anim::Animation> _addedAnimations;
  std::vector<util::Uuid> _removedAnimations;

  std::vector<component::Renderable> _addedRenderables;
  std::vector<component::Renderable> _updatedRenderables;
  std::vector<util::Uuid> _removedRenderables;

  std::vector<component::Light> _addedLights;
  std::vector<component::Light> _updatedLights;
  std::vector<util::Uuid> _removedLights;

  std::vector<asset::TileInfo> _addedTileInfos;
  std::vector<asset::TileInfo> _updatedTileInfos;
  std::vector<scene::TileIndex> _removedTileInfos;
};

class RenderContext
{
public:
  virtual ~RenderContext() {}

  virtual bool isBaking() = 0;

  // Thread-safe but NOT performant. Meant to be used when importing.
  virtual void generateMipMaps(asset::Texture& tex) = 0;

  virtual VkDevice& device() = 0;
  virtual VkDescriptorPool& descriptorPool() = 0;
  virtual VmaAllocator vmaAllocator() = 0;

  virtual VkPipelineLayout& bindlessPipelineLayout() = 0;
  virtual VkDescriptorSetLayout& bindlessDescriptorSetLayout() = 0;
  virtual VkExtent2D swapChainExtent() = 0;

  virtual std::size_t getGigaBufferSizeMB() = 0;

  virtual VkDeviceAddress getGigaVtxBufferAddr() = 0;
  virtual VkDeviceAddress getGigaIdxBufferAddr() = 0;

  virtual void drawGigaBufferIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount) = 0;
  virtual void drawGigaBufferIndirectCount(VkCommandBuffer*, VkBuffer drawCalls, VkBuffer count, uint32_t maxDrawCount) = 0;
  virtual void drawNonIndexIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount, uint32_t stride) = 0;
  virtual void drawMeshId(VkCommandBuffer*, util::Uuid, uint32_t vertCount, uint32_t instanceCount) = 0;

  virtual VkImage& getCurrentSwapImage() = 0;
  virtual int getCurrentMultiBufferIdx() = 0;
  virtual int getMultiBufferSize() = 0;

  virtual void assetUpdate(AssetUpdate&& update) = 0;

  virtual size_t getMaxNumMeshes() = 0;
  virtual size_t getMaxNumRenderables() = 0;
  virtual size_t getMaxBindlessResources() = 0;
  virtual size_t getMaxNumPointLightShadows() = 0;

  virtual const std::vector<internal::InternalLight>& getLights() = 0;
  virtual std::vector<int> getShadowCasterLightIndices() = 0;

  virtual std::vector<std::size_t> getTerrainIndices() = 0;

  virtual std::vector<internal::InternalMesh>& getCurrentMeshes() = 0;
  virtual std::vector<internal::InternalRenderable>& getCurrentRenderables() = 0;
  virtual bool getRenderableById(util::Uuid id, internal::InternalRenderable** out) = 0;
  virtual bool getMeshById(util::Uuid id, internal::InternalMesh** out) = 0;
  virtual std::unordered_map<util::Uuid, std::size_t>& getCurrentMeshUsage() = 0;
  virtual size_t getCurrentNumRenderables() = 0;

  virtual std::unordered_map<util::Uuid, std::vector<AccelerationStructure>>& getDynamicBlases() = 0;

  virtual gpu::GPUCullPushConstants getCullParams() = 0;

  virtual RenderDebugOptions& getDebugOptions() = 0;
  virtual RenderOptions& getRenderOptions() = 0;

  virtual void setDebugName(VkObjectType objectType, uint64_t objectHandle, const char* name) = 0;

  virtual glm::vec2 getWindDir() = 0;

  virtual VkCommandBuffer beginSingleTimeCommands() = 0;
  virtual void endSingleTimeCommands(VkCommandBuffer buffer) = 0;

  virtual VkPhysicalDeviceRayTracingPipelinePropertiesKHR getRtPipeProps() = 0;

  virtual void registerPerFrameTimer(const std::string& name, const std::string& group) = 0;
  virtual void startTimer(const std::string& name, VkCommandBuffer cmdBuffer) = 0;
  virtual void stopTimer(const std::string& name, VkCommandBuffer cmdBuffer) = 0;

  virtual internal::InternalMesh& getSphereMesh() = 0;

  virtual size_t getNumIrradianceProbesXZ() = 0;
  virtual size_t getNumIrradianceProbesY() = 0;

  virtual double getElapsedTime() = 0;

  virtual const Camera& getCamera() = 0;

  virtual AccelerationStructure& getTLAS() = 0;

  virtual VkFormat getHDRFormat() = 0;

  // Hack for testing
  virtual std::vector<Particle>& getParticles() = 0;

  // TODO: A proper blackboard
  virtual bool blackboardValueBool(const std::string& key) = 0;
  virtual int blackboardValueInt(const std::string& key) = 0;
  virtual void setBlackboardValueBool(const std::string& key, bool val) = 0;
  virtual void setBlackboardValueInt(const std::string& key, int val) = 0;
};

}