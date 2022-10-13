#pragma once

#include "AllocatedImage.h"
#include "Model.h"
#include "ImageHelpers.h"

#include "../util/GraphicsUtils.h"

#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>

namespace render {

struct PostProcessingPass
{
  void beginRendering(VkCommandBuffer cmdBuffer, VkImageView& imageView, VkExtent2D& extent)
  {
    VkClearValue clearValue;
    clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

    VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachmentInfo.imageView = imageView;
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.clearValue = clearValue;

    VkRenderingInfoKHR renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderInfo.renderArea.extent = extent;
    renderInfo.renderArea.offset = {0, 0};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachmentInfo;
    renderInfo.pDepthAttachment = nullptr;

    vkCmdBeginRendering(cmdBuffer, &renderInfo);
  }

  VkViewport viewport(int width, int height)
  {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
  }

  VkRect2D scissor(VkExtent2D extent)
  {
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    return scissor;
  }

  void cleanup(VmaAllocator vmaAllocator, VkDevice device)
  {
    for (auto& resource: _ppResources) {
      vkDestroyImageView(device, resource._ppInputImageView, nullptr);
      vmaDestroyImage(vmaAllocator, resource._ppInputImage._image, resource._ppInputImage._allocation);
      vkDestroySampler(device, resource._ppSampler, nullptr);
    }
  }

  void init(VkFormat imageFormat, int width, int height)
  {
    _width = width;
    _height = height;
    _ppImageFormat = imageFormat;
    _quadModel._vertices = graphicsutil::createScreenQuad(1.0f, 1.0f);
  }

  bool createResources(VkDevice device, VmaAllocator vmaAllocator, VkCommandBuffer cmdBuffer, std::size_t* resourceIndexOut)
  {
    PPResource resource;
    imageutil::createImage(_width, _height, _ppImageFormat, VK_IMAGE_TILING_OPTIMAL, vmaAllocator, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, resource._ppInputImage);
    resource._ppInputImageView = imageutil::createImageView(device, resource._ppInputImage._image, _ppImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    imageutil::transitionImageLayout(cmdBuffer, resource._ppInputImage._image, _ppImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

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

    if (vkCreateSampler(device, &sampler, nullptr, &resource._ppSampler) != VK_SUCCESS) {
      printf("Could not create shadow map sampler!\n");
      return false;
    }

    _ppResources.emplace_back(std::move(resource));
    *resourceIndexOut = _ppResources.size() - 1;

    return true;
  }

  std::int64_t _quadModelId;
  Model _quadModel;

  int _width;
  int _height;

  struct PPResource {
    AllocatedImage _ppInputImage;
    VkImageView _ppInputImageView;
    VkSampler _ppSampler;
  };

  std::vector<PPResource> _ppResources;

  VkFormat _ppImageFormat;
};

}