#include "LuminanceAveragePass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"

namespace render {

namespace {

void initLumBuf(RenderContext* renderContext, AllocatedBuffer& buf)
{
  auto cmdBuf = renderContext->beginSingleTimeCommands();

  vkCmdFillBuffer(
    cmdBuf,
    buf._buffer,
    0,
    VK_WHOLE_SIZE,
    0);

  renderContext->endSingleTimeCommands(cmdBuf);
}

}

LuminanceAveragePass::LuminanceAveragePass()
  : RenderPass()
{}

LuminanceAveragePass::~LuminanceAveragePass()
{}

void LuminanceAveragePass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "LuminanceAverage";

  {
    ResourceUsage usage{};
    usage._resourceName = "HistogramSSBO";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "AvgLumSSBO";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);
    //usage._stage.set((std::size_t)Stage::Transfer);
    usage._type = Type::SSBO;

    BufferInitialCreateInfo createInfo{};
    createInfo._initialSize = 1 * sizeof(float); // Just one average value
    createInfo._initialDataCb = initLumBuf;

    usage._bufferCreateInfo = createInfo;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams param{};
  param.device = rc->device();
  param.shader = "luminance_average_comp.spv";
  info._computeParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("LuminanceAverage",
    [this](RenderExeParams exeParams) {
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      vkCmdDispatch(*exeParams.cmdBuffer, 1, 1, 1);
    });
}

}