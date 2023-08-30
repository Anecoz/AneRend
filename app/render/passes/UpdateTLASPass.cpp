#include "UpdateTLASPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../VulkanExtensions.h"
#include "../RenderResource.h"

namespace render {

UpdateTLASPass::UpdateTLASPass()
  : RenderPass()
{}

UpdateTLASPass::~UpdateTLASPass()
{}

void UpdateTLASPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Add initializers for the buffers (fill with 0's)
  {
    ResourceUsage initUsage{};
    initUsage._type = Type::SSBO;
    initUsage._access.set((std::size_t)Access::Write);
    initUsage._stage.set((std::size_t)Stage::Transfer);

    fgb.registerResourceInitExe("TLASInstanceBuf", std::move(initUsage),
      [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
        auto buf = (BufferRenderResource*)resource;

        vkCmdFillBuffer(
          cmdBuffer,
          buf->_buffer._buffer,
          0,
          VK_WHOLE_SIZE,
          0);
      });
  }
  {
    ResourceUsage initUsage{};
    initUsage._type = Type::SSBO;
    initUsage._access.set((std::size_t)Access::Write);
    initUsage._stage.set((std::size_t)Stage::Transfer);

    fgb.registerResourceInitExe("TLASBuildRangeBuf", std::move(initUsage),
      [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
        auto buf = (BufferRenderResource*)resource;

        vkCmdFillBuffer(
          cmdBuffer,
          buf->_buffer._buffer,
          0,
          VK_WHOLE_SIZE,
          0);
      });
  }

  RenderPassRegisterInfo info{};
  info._name = "TLASUpdate";

  {
    ResourceUsage usage{};
    usage._resourceName = "TLASInstanceBuf";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;

    BufferInitialCreateInfo createInfo{};
    createInfo._initialSize = rc->getMaxNumRenderables() * sizeof(VkAccelerationStructureInstanceKHR);
    createInfo._flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    usage._bufferCreateInfo = createInfo;

    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "TLASBuildRangeBuf";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;

    BufferInitialCreateInfo createInfo{};
    createInfo._initialSize = sizeof(VkAccelerationStructureBuildRangeInfoKHR);
    usage._bufferCreateInfo = createInfo;

    info._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams param{};
  param.device = rc->device();
  param.shader = "tlas_update_comp.spv";
  info._computeParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("TLASUpdate",
    [this](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().raytracingEnabled) return;

      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      uint32_t maxPrimitiveCount = exeParams.rc->getCurrentNumRenderables();
      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        0,
        sizeof(uint32_t),
        &maxPrimitiveCount);

      uint32_t localSize = 8;
      uint32_t numX = maxPrimitiveCount / localSize;

      vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, 1, 1);

      // Manual memory barrier here since it's not part of the frame graph...
      VkBufferMemoryBarrier instanceBarrier{};
      instanceBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      instanceBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      instanceBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
      instanceBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      instanceBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      instanceBarrier.buffer = exeParams.buffers[0];
      instanceBarrier.offset = 0;
      instanceBarrier.size = VK_WHOLE_SIZE;

      VkBufferMemoryBarrier buildRangeBarrier{};
      buildRangeBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      buildRangeBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      buildRangeBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
      buildRangeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      buildRangeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      buildRangeBarrier.buffer = exeParams.buffers[1];
      buildRangeBarrier.offset = 0;
      buildRangeBarrier.size = VK_WHOLE_SIZE;

      std::vector<VkBufferMemoryBarrier> barriers {instanceBarrier, buildRangeBarrier};

      vkCmdPipelineBarrier(*exeParams.cmdBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0,
        0, nullptr,
        2, barriers.data(),
        0, nullptr);

      // Build the TLAS
      VkBufferDeviceAddressInfo instanceAddrInfo{};
      instanceAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
      instanceAddrInfo.buffer = exeParams.buffers[0];

      VkDeviceAddress instanceAddr = vkGetBufferDeviceAddress(exeParams.rc->device(), &instanceAddrInfo);

      VkAccelerationStructureGeometryInstancesDataKHR instancesVk{};
      instancesVk.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      instancesVk.arrayOfPointers = VK_FALSE;
      instancesVk.data.deviceAddress = instanceAddr;

      VkAccelerationStructureGeometryKHR geometry{};
      geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
      geometry.geometry.instances = instancesVk;

      auto& tlas = exeParams.rc->getTLAS();

      VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
      buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
      buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      buildInfo.geometryCount = 1;
      buildInfo.pGeometries = &geometry;
      buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
      buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
      buildInfo.dstAccelerationStructure = tlas._as;
      buildInfo.scratchData.deviceAddress = tlas._scratchAddress;

      VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
      rangeInfo.primitiveCount = maxPrimitiveCount;

      auto rangeInfoPtr = &rangeInfo;

      vkext::vkCmdBuildAccelerationStructuresKHR(
        *exeParams.cmdBuffer,
        1, 
        &buildInfo,
        &rangeInfoPtr);

      // Protect TLAS buffer
      VkBufferMemoryBarrier postBarrier{};
      postBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      postBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
      postBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
      postBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      postBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      postBarrier.buffer = tlas._buffer._buffer;
      postBarrier.offset = 0;
      postBarrier.size = VK_WHOLE_SIZE;

      vkCmdPipelineBarrier(*exeParams.cmdBuffer,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        0,
        0, nullptr,
        1, &postBarrier,
        0, nullptr);
    });
}

}