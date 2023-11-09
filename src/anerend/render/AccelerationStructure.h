#pragma once

#include "AllocatedBuffer.h"

namespace render {

struct AccelerationStructure
{
  explicit operator bool() const {
    return _accelerationStructureSize > 0;
  }

  AllocatedBuffer _buffer;
  AllocatedBuffer _scratchBuffer;
  VkDeviceAddress _scratchAddress;
  VkDeviceSize _scratchBufferSize;
  VkAccelerationStructureKHR _as;
  VkDeviceSize _accelerationStructureSize = 0;
};

}