#pragma once

#include "Material.h"

namespace render {

class MaterialFactory
{
public:
  static Material createStandardMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout imageLayout);

  static Material createStandardInstancedMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout imageLayout);

  static Material createShadowDebugMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout imageLayout);
};

}