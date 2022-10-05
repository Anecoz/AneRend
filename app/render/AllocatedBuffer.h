#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace render {

struct AllocatedBuffer {
  VkBuffer _buffer;
  VmaAllocation _allocation;
};

}

