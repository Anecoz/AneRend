#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace render {

struct AllocatedImage {
  VkImage _image;
  VmaAllocation _allocation;
};

}