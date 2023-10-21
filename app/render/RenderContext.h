#pragma once

#include <vulkan/vulkan.h>

#include "vma/vk_mem_alloc.h"

#include "asset/Model.h"
#include "asset/Renderable.h"
#include "asset/Material.h"
#include "animation/Animation.h"
#include "animation/Skeleton.h"
#include "internal/InternalMesh.h"
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
  std::vector<asset::Model> _addedModels;
  std::vector<ModelId> _removedModels;

  std::vector<asset::Material> _addedMaterials;
  std::vector<MaterialId> _removedMaterials;

  std::vector<anim::Animation> _addedAnimations;
  std::vector<AnimationId> _removedAnimations;

  std::vector<anim::Skeleton> _addedSkeletons;
  std::vector<SkeletonId> _removedSkeletons;

  std::vector<asset::Renderable> _addedRenderables;
  std::vector<asset::Renderable> _updatedRenderables;
  std::vector<RenderableId> _removedRenderables;
};

class RenderContext
{
public:
  virtual ~RenderContext() {}

  virtual VkDevice& device() = 0;
  virtual VkDescriptorPool& descriptorPool() = 0;
  virtual VmaAllocator vmaAllocator() = 0;

  virtual VkPipelineLayout& bindlessPipelineLayout() = 0;
  virtual VkDescriptorSetLayout& bindlessDescriptorSetLayout() = 0;
  virtual VkExtent2D swapChainExtent() = 0;

  virtual void drawGigaBufferIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount) = 0;
  virtual void drawNonIndexIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount, uint32_t stride) = 0;
  virtual void drawMeshId(VkCommandBuffer*, MeshId, uint32_t vertCount, uint32_t instanceCount) = 0;

  virtual VkImage& getCurrentSwapImage() = 0;
  virtual int getCurrentMultiBufferIdx() = 0;
  virtual int getMultiBufferSize() = 0;

  virtual void assetUpdate(AssetUpdate&& update) = 0;

  virtual size_t getMaxNumMeshes() = 0;
  virtual size_t getMaxNumRenderables() = 0;

  virtual size_t getMaxBindlessResources() = 0;

  virtual std::vector<internal::InternalMesh>& getCurrentMeshes() = 0;
  virtual std::unordered_map<MeshId, std::size_t>& getCurrentMeshUsage() = 0;
  virtual size_t getCurrentNumRenderables() = 0;

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