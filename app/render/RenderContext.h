#pragma once

#include <vulkan/vulkan.h>

#include "vma/vk_mem_alloc.h"

#include "Mesh.h"
#include "Vertex.h"
#include "RenderableId.h"
#include "GpuBuffers.h"
#include "RenderDebugOptions.h"

#include <unordered_map>

namespace render {

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

  //virtual void bindGigaBuffers(VkCommandBuffer*) = 0;
  virtual void drawGigaBuffer(VkCommandBuffer*) = 0;
  virtual void drawGigaBufferIndirect(VkCommandBuffer*, VkBuffer drawCalls) = 0;
  virtual void drawNonIndexIndirect(VkCommandBuffer*, VkBuffer drawCalls, uint32_t drawCount, uint32_t stride) = 0;
  virtual void drawMeshId(VkCommandBuffer*, MeshId, uint32_t vertCount, uint32_t instanceCount) = 0;

  virtual VkImage& getCurrentSwapImage() = 0;
  virtual int getCurrentMultiBufferIdx() = 0;
  virtual int getMultiBufferSize() = 0;

  virtual MeshId registerMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices) = 0;
  virtual RenderableId registerRenderable(
    MeshId meshId,
    const glm::mat4& transform,
    const glm::vec3& sphereBoundCenter,
    float sphereBoundRadius) = 0;
  virtual void setRenderableVisible(RenderableId id, bool visible) = 0;

  virtual size_t getMaxNumMeshes() = 0;
  virtual size_t getMaxNumRenderables() = 0;

  virtual std::vector<Mesh>& getCurrentMeshes() = 0;
  virtual std::unordered_map<MeshId, std::size_t>& getCurrentMeshUsage() = 0;
  virtual size_t getCurrentNumRenderables() = 0;

  virtual gpu::GPUCullPushConstants getCullParams() = 0;

  virtual RenderDebugOptions& getDebugOptions() = 0;

  virtual void setDebugName(VkObjectType objectType, uint64_t objectHandle, const char* name) = 0;

  virtual glm::vec2 getWindDir() = 0;
};

}