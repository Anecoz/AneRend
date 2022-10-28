#pragma once

#include "Material.h"

#include <vector>

namespace render {

class MaterialFactory
{
public:
  static Material createStandardMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    const std::vector<AllocatedBuffer>& gpuRenderableBuffers, const std::vector<AllocatedBuffer>& gpuTranslationBuffers,
    const std::vector<VkImageView*>& textureImageViews, const std::vector<VkSampler*>& samplers, VkImageLayout samplerImageLayout);

  static Material createStandardInstancedMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    const std::vector<VkImageView*>& textureImageViews, const std::vector<VkSampler*>& samplers, VkImageLayout samplerImageLayout);

  static Material createShadowDebugMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout);

  static Material createPPColorInvMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout);

  static Material createPPFlipMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout);

  static Material createPPBlurMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout);

  static Material createPPFxaaMaterial(
    VkDevice device, VkFormat colorFormat, VkFormat depthFormat, std::size_t numCopies,
    VkDescriptorPool& descriptorPool, VmaAllocator& vmaAllocator,
    VkImageView* imageView, VkSampler* sampler, VkImageLayout samplerImageLayout);
};

}