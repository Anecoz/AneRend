#include "SSAOBlurRenderPass.h"

#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../FrameGraphBuilder.h"
#include "../ImageHelpers.h"
#include "../../util/GraphicsUtils.h"

namespace render {

SSAOBlurRenderPass::SSAOBlurRenderPass()
  : RenderPass()
{}

SSAOBlurRenderPass::~SSAOBlurRenderPass()
{}

void SSAOBlurRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "SSAOBlur";

  std::vector<ResourceUsage> resourceUsages;
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOBlurImage";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialHeight = rc->swapChainExtent().height;
    createInfo._initialWidth = rc->swapChainExtent().width;
    createInfo._intialFormat = VK_FORMAT_R32_SFLOAT;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    resourceUsages.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsages);

  ComputePipelineCreateParams param{};
  param.device = rc->device();
  param.shader = "ssao_blur_comp.spv";
  info._computeParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("SSAOBlur",
    [this](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().ssao) return;

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