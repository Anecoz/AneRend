#include "SurfelConvolveRenderPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"

namespace render {

SurfelConvolveRenderPass::SurfelConvolveRenderPass()
  : RenderPass()
{}

SurfelConvolveRenderPass::~SurfelConvolveRenderPass()
{}

void SurfelConvolveRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "SurfelConvolve";

  const int numSurfelsX = std::ceil(rc->swapChainExtent().width / 32.0);
  const int numSurfelsY = std::ceil(rc->swapChainExtent().height / 32.0);
  const int octSize = 8;

  {
    ResourceUsage usage{};
    usage._resourceName = "SurfelTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    //usage._samplerClamp = true;
    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SurfelConvTex";
    usage._access.set((std::size_t)Access::Read);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = numSurfelsX * octSize + numSurfelsX * 2; // 1 pix border around each cell
    createInfo._initialHeight = numSurfelsY * octSize + numSurfelsY * 2; // ditto
    createInfo._intialFormat = VK_FORMAT_R8G8B8A8_UNORM;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams pipeParam{};
  pipeParam.device = rc->device();
  pipeParam.shader = "surfel_conv_comp.spv";

  info._computeParams = pipeParam;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("SurfelConvolve",
    [this, numSurfelsX, numSurfelsY, octSize](RenderExeParams exeParams) {
      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      auto extent = exeParams.rc->swapChainExtent();

      uint32_t numX = numSurfelsX * octSize;
      numX = numX / 8;
      uint32_t numY = numSurfelsY * octSize;
      numY = numY / 8;

      vkCmdDispatch(*exeParams.cmdBuffer, numX, numY + 1, 1);
    });
}

}