#include "LuminanceHistogramPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../RenderResource.h"

namespace render {

LuminanceHistogramPass::LuminanceHistogramPass()
  : RenderPass()
{}

LuminanceHistogramPass::~LuminanceHistogramPass()
{}

void LuminanceHistogramPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Create a 256-bin histogram of the luminance of the finished HDR image

  RenderPassRegisterInfo info{};
  info._name = "LuminanceHistogram";

  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry2Image";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "HistogramSSBO";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;

    BufferInitialCreateInfo createInfo{};
    createInfo._initialSize = 256 * sizeof(std::uint32_t); // 256 luminance bins

    usage._bufferCreateInfo = createInfo;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  {
    // Add an initializer for the buffer
    ResourceUsage initUsage{};
    initUsage._type = Type::SSBO;
    initUsage._access.set((std::size_t)Access::Write);
    initUsage._stage.set((std::size_t)Stage::Transfer);

    fgb.registerResourceInitExe("HistogramSSBO", std::move(initUsage),
      [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
        // Just fill buffer with 0's
        auto buf = (BufferRenderResource*)resource;

        vkCmdFillBuffer(
          cmdBuffer,
          buf->_buffer._buffer,
          0,
          VK_WHOLE_SIZE,
          0);
      });
  }

  ComputePipelineCreateParams param{};
  param.device = rc->device();
  param.shader = "luminance_histogram_comp.spv";
  info._computeParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("LuminanceHistogram",
    [this](RenderExeParams exeParams) {
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      uint32_t localSize = 16;
      uint32_t numY = exeParams.rc->swapChainExtent().height / localSize;
      uint32_t numX = exeParams.rc->swapChainExtent().width / localSize;

      vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, numY + 1, 1);
    });
}

}