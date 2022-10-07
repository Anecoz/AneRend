#pragma once

#include <vulkan/vulkan.h>

namespace render {

// Material is these vulkan objects:
// - VkPipeline (describes input assembly, rasterizer, shader stages, multisampling, depth and stencil testing etc etc)
// - VkDescriptorSetLayout (describes layout of resource descriptors, like ubos or samplers)
// - VkPipelineLayout (describes VkDescriptorSetLayout + push constants and stuff)
// This should all be fixed (fixed per frame per material)
struct Material
{
  VkPipeline _pipeline = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

  VkPipeline _pipelineShadow = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayoutShadow = VK_NULL_HANDLE;
  VkDescriptorSetLayout _descriptorSetLayoutShadow = VK_NULL_HANDLE;

  bool _hasInstanceBuffer = false;
  bool _supportsPushConstants = false;
  bool _supportsShadowPass = false;

  explicit inline operator bool() const {
    return
      _pipeline != VK_NULL_HANDLE &&
      _pipelineLayout != VK_NULL_HANDLE &&
      _descriptorSetLayout != VK_NULL_HANDLE;
  }
};

}