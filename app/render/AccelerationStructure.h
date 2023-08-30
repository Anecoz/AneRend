#pragma once

#include "AllocatedBuffer.h"

namespace render {

struct AccelerationStructure
{
  AllocatedBuffer _buffer;
  AllocatedBuffer _scratchBuffer;
  VkDeviceAddress _scratchAddress;
  VkAccelerationStructureKHR _as;
};

}