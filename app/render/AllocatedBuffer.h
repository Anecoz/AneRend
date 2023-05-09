#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace render {

struct AllocatedBuffer {
  VkBuffer _buffer = VK_NULL_HANDLE;
  VmaAllocation _allocation;
};

}

