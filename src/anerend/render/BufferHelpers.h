#pragma once

#include "AllocatedBuffer.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>

namespace render {
namespace bufferutil {

static void createBuffer(VmaAllocator& allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags properties, AllocatedBuffer& buffer)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  vmaAllocInfo.flags = properties;

  if (vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo,
              &buffer._buffer,
              &buffer._allocation,
              nullptr) != VK_SUCCESS) {
     printf("Could not create vma buffer!\n");
  }
}

}}