#pragma once

#include "vk_mem_alloc.h"
#include "ImageHelpers.h"
#include "Model.h"

#include "../util/GraphicsUtils.h"

#include <vulkan/vulkan.h>

namespace render {

struct Shadowpass
{
  void beginRendering(VkCommandBuffer cmdBuffer)
  {
    VkClearValue clearValue;
    clearValue.depthStencil = {1.0f, 0};

    VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
    depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    depthAttachmentInfo.imageView = _shadowImageView;
    depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachmentInfo.clearValue = clearValue;

    VkRenderingInfoKHR renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderInfo.renderArea.extent = _shadowExtent;
    renderInfo.renderArea.offset = {0, 0};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 0;
    renderInfo.pDepthAttachment = &depthAttachmentInfo;

    vkCmdBeginRendering(cmdBuffer, &renderInfo);
  }

  void viewport(VkCommandBuffer cmdBuffer)
  {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_shadowExtent.width);
    viewport.height = static_cast<float>(_shadowExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
  }

  void scissor(VkCommandBuffer cmdBuffer)
  {
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _shadowExtent;
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
  }

  void cleanup(VmaAllocator vmaAllocator, VkDevice device)
  {
    vkDestroyImageView(device, _shadowImageView, nullptr);
    vmaDestroyImage(vmaAllocator, _shadowImage._image, _shadowImage._allocation);
    vkDestroySampler(device, _shadowSampler, nullptr);
  }

  bool createShadowResources(VkDevice device, VmaAllocator vmaAllocator, VkCommandBuffer cmdBuffer, VkFormat depthFormat, int width, int height)
  {
    _shadowExtent.height = height;
    _shadowExtent.width = width;

    imageutil::createImage(width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL, vmaAllocator, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _shadowImage);
    _shadowImageView = imageutil::createImageView(device, _shadowImage._image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    imageutil::transitionImageLayout(cmdBuffer, _shadowImage._image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (vkCreateSampler(device, &sampler, nullptr, &_shadowSampler) != VK_SUCCESS) {
      printf("Could not create shadow map sampler!\n");
      return false;
    }

    _debugModel._vertices = graphicsutil::createScreenQuad();

    return true;
  }

  std::int64_t _debugModelId;
  Model _debugModel;
  VkExtent2D _shadowExtent;
  AllocatedImage _shadowImage;
  VkImageView _shadowImageView;
  VkSampler _shadowSampler;
};

}