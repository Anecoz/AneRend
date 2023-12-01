#include "CompactDrawsPass.h"

#include "../GpuBuffers.h"
#include "../BufferHelpers.h"
#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../FrameGraphBuilder.h"

namespace render {

CompactDrawsPass::CompactDrawsPass()
  : RenderPass()
{}

CompactDrawsPass::~CompactDrawsPass()
{}

void CompactDrawsPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
 
  // Add an initializer for the counter
  {
    ResourceUsage initUsage{};
    initUsage._type = Type::SSBO;
    initUsage._access.set((std::size_t)Access::Write);
    initUsage._stage.set((std::size_t)Stage::Transfer);

    fgb.registerResourceInitExe("CompactDrawCountBuf", std::move(initUsage),
      [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
        auto buf = (BufferRenderResource*)resource;
        vkCmdFillBuffer(cmdBuffer, buf->_buffer._buffer, 0, VK_WHOLE_SIZE, 0);
      });
  }

  // Register the actual render pass
  RenderPassRegisterInfo regInfo{};
  regInfo._name = "CompactCull";

  {
    ResourceUsage usage{};
    usage._resourceName = "CullDrawBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;
    usage._multiBuffered = true;
    regInfo._resourceUsages.emplace_back(std::move(usage));
  }

  {
    ResourceUsage usage{};
    usage._resourceName = "CompactedCullDrawBuf";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;
    BufferInitialCreateInfo drawCreateInfo{};
    drawCreateInfo._multiBuffered = true;
    drawCreateInfo._initialSize = rc->getMaxNumMeshes() * sizeof(gpu::GPUDrawCallCmd);
    usage._bufferCreateInfo = drawCreateInfo;
    regInfo._resourceUsages.emplace_back(std::move(usage));
  }

  {
    ResourceUsage usage{};
    usage._resourceName = "CompactDrawCountBuf";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;
    BufferInitialCreateInfo ci{};
    ci._multiBuffered = true;
    ci._initialSize = sizeof(uint32_t);
    usage._bufferCreateInfo = ci;
    regInfo._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams pipeParam{};
  pipeParam.device = rc->device();
  pipeParam.shader = "compact_draws_comp.spv";

  regInfo._computeParams = pipeParam;

  fgb.registerRenderPass(std::move(regInfo));
  fgb.registerRenderPassExe("CompactCull",
    [this](RenderExeParams exeParams)
    {
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      // Bind to set 1
      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[exeParams.rc->getCurrentMultiBufferIdx()],
        0, nullptr);

      uint32_t maxDrawCount = static_cast<uint32_t>(exeParams.rc->getCurrentMeshes().size());
      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        0,
        sizeof(uint32_t),
        &maxDrawCount);

      const uint32_t localSize = 8;
      uint32_t numX = maxDrawCount / localSize;
      vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, 1, 1);
    });
}

}