#include "MipMapGenerator.h"

#include "../RenderContext.h"
#include "../asset/Texture.h"
#include "../ImageHelpers.h"
#include "../BufferHelpers.h"

namespace render::internal {

namespace {

// TODO: Currently only deals with ASPECT_COLOR (not depth) and only single layer images
void internalGenerate(
  VkCommandBuffer cmdBuffer,
  VkImage image,
  VkFormat imageFormat,
  int32_t texWidth,
  int32_t texHeight,
  uint32_t mipLevels)
{
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    VkImageBlit blit{};
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(cmdBuffer,
      image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit,
      VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  // Transition all mips to src
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mipLevels;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(cmdBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
}

}

void MipMapGenerator::generateMipMaps(render::asset::Texture& tex, render::RenderContext* rc)
{
  // 1. Create a host-accessible VkImage
  // 2. Fill with level 0 data present in tex
  // 3. Generate mipmaps using internalGenerate
  // 4. Copy generated mip levels from VkImage back to tex

  // 1. Create host-accessible VkImage
  int width = (int)tex._width;
  int height = (int)tex._height;
  auto format = imageutil::texFormatToVk(tex._format);
  uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
  render::AllocatedImage image;

  imageutil::createImage(
    width, height,
    format,
    VK_IMAGE_TILING_OPTIMAL,
    rc->vmaAllocator(),
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    image,
    mipLevels);

  // Create a staging buffer
  render::AllocatedBuffer stagingBuf;
  auto allocator = rc->vmaAllocator();
  bufferutil::createBuffer(
    allocator,
    width * height * 4 * 2,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    stagingBuf);

  // 2. Map and write data to staging buf
  uint8_t* mappedData;
  vmaMapMemory(rc->vmaAllocator(), stagingBuf._allocation, &(void*)mappedData);
  std::memcpy(mappedData, tex._data[0].data(), tex._data[0].size());
  vmaUnmapMemory(rc->vmaAllocator(), stagingBuf._allocation);

  auto cmdBuffer = rc->beginSingleTimeCommands();

  // Transition image to dst optimal
  imageutil::transitionImageLayout(
    cmdBuffer,
    image._image,
    format,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    0,
    mipLevels);

  // Do the copy
  VkExtent3D extent{};
  extent.depth = 1;
  extent.height = height;
  extent.width = width;

  VkOffset3D offset{};
  VkBufferImageCopy imCopy{};
  imCopy.imageExtent = extent;
  imCopy.imageOffset = offset;
  imCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imCopy.imageSubresource.layerCount = 1;

  vkCmdCopyBufferToImage(
    cmdBuffer,
    stagingBuf._buffer,
    image._image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &imCopy);

  // 3. Generate mips
  internalGenerate(cmdBuffer, image._image, format, width, height, mipLevels);
  rc->endSingleTimeCommands(cmdBuffer);

  // 4. Copy back to cpu
  int32_t mipWidth = width / 2;
  int32_t mipHeight = height / 2;
  for (uint32_t i = 1; i < mipLevels; ++i) {
    auto size = imageutil::texSize(mipWidth, mipHeight, tex._format);

    auto& currTexData = tex._data.emplace_back();
    currTexData.resize(size);

    auto cmdBuf = rc->beginSingleTimeCommands();

    VkBufferImageCopy imCopy{};
    imCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imCopy.imageSubresource.layerCount = 1;
    imCopy.imageSubresource.mipLevel = i;
    imCopy.imageExtent.depth = 1;
    imCopy.imageExtent.height = mipHeight;
    imCopy.imageExtent.width = mipWidth;
    vkCmdCopyImageToBuffer(cmdBuf, image._image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuf._buffer, 1, &imCopy);

    rc->endSingleTimeCommands(cmdBuf);

    void* mappedData = nullptr;
    vmaMapMemory(rc->vmaAllocator(), stagingBuf._allocation, &(void*)mappedData);
    std::memcpy(currTexData.data(), mappedData, size);
    vmaUnmapMemory(rc->vmaAllocator(), stagingBuf._allocation);

    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
    tex._numMips++;
  }

  // Cleanup
  vmaDestroyImage(rc->vmaAllocator(), image._image, image._allocation);
  vmaDestroyBuffer(rc->vmaAllocator(), stagingBuf._buffer, stagingBuf._allocation);
}

}