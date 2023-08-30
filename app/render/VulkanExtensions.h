#pragma once

#include <vulkan/vulkan.h>

namespace vkext {
  extern PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
  extern PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
  extern PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
  extern PFN_vkCmdBuildAccelerationStructuresIndirectKHR vkCmdBuildAccelerationStructuresIndirectKHR;
  extern PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;

  void vulkanExtensionInit(VkDevice device);
}