#pragma once

#include <vulkan/vulkan.h>

#include "AllocatedBuffer.h"

namespace render {

struct ShaderBindingTable
{
  AllocatedBuffer _buffer;
  VkStridedDeviceAddressRegionKHR _rgenRegion;
  VkStridedDeviceAddressRegionKHR _missRegion;
  VkStridedDeviceAddressRegionKHR _chitRegion;
};

}