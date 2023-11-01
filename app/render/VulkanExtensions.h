#pragma once

#include <vulkan/vulkan.h>

namespace vkext {
  extern PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
  extern PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
  extern PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
  extern PFN_vkCmdBuildAccelerationStructuresIndirectKHR vkCmdBuildAccelerationStructuresIndirectKHR;
  extern PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
  extern PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR;

  void vulkanExtensionInit(VkDevice device);
}