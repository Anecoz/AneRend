#pragma once

#include "AllocatedBuffer.h"
#include "StandardUBO.h"
#include "ShadowUBO.h"

#include "vk_mem_alloc.h"

#include <vulkan/vulkan.h>

#include <array>
#include <vector>

namespace render {

// Material is these vulkan objects:
// - VkPipeline (describes input assembly, rasterizer, shader stages, multisampling, depth and stencil testing etc etc)
// - VkDescriptorSetLayout (describes layout of resource descriptors, like ubos or samplers)
// - VkPipelineLayout (describes VkDescriptorSetLayout + push constants and stuff)
// This should all be fixed (fixed per frame per material)
struct Material
{
  static const std::size_t STANDARD_INDEX = 0;
  static const std::size_t SHADOW_INDEX = 1;
  static const std::size_t POST_PROCESSING_INDEX = 2;

  std::array<VkPipeline, 3> _pipelines = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};
  std::array<VkPipelineLayout, 3> _pipelineLayouts = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};
  std::array<VkDescriptorSetLayout, 3> _descriptorSetLayouts = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};

  // Several copies, to account for multiple swap chain images etc (triple buffering for instance)
  std::array<std::vector<VkDescriptorSet>, 3> _descriptorSets;
  std::array<std::vector<AllocatedBuffer>, 3> _ubos;

  void updateUbos(std::uint32_t index, VmaAllocator& vmaAllocator, const StandardUBO& standardUbo, const ShadowUBO& shadowUbo);

  bool _hasInstanceBuffer = false;
  bool _supportsPushConstants = false;
  bool _supportsShadowPass = false;
  bool _supportsPostProcessingPass = false;

  explicit inline operator bool() const {
    return
      (_pipelines[STANDARD_INDEX] != VK_NULL_HANDLE &&
      _pipelineLayouts[STANDARD_INDEX] != VK_NULL_HANDLE &&
      _descriptorSetLayouts[STANDARD_INDEX] != VK_NULL_HANDLE)
      ||
      (_supportsPostProcessingPass &&
      _pipelines[POST_PROCESSING_INDEX] != VK_NULL_HANDLE &&
      _pipelineLayouts[POST_PROCESSING_INDEX] != VK_NULL_HANDLE &&
      _descriptorSetLayouts[POST_PROCESSING_INDEX] != VK_NULL_HANDLE);
  }
};

}