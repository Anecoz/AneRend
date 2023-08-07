#include "IrradianceProbeConvolvePass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"

namespace render {

IrradianceProbeConvolvePass::IrradianceProbeConvolvePass()
  : RenderPass()
{}

IrradianceProbeConvolvePass::~IrradianceProbeConvolvePass()
{}

void IrradianceProbeConvolvePass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "IrradianceProbeConvolve";

  const int octPixelSize = 8;
  const int numProbesPlane = rc->getNumIrradianceProbesXZ();
  const int numProbesHeight = rc->getNumIrradianceProbesY();

  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysIrrTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysDirTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysConvTex";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = (octPixelSize + 2) * numProbesPlane; // 1 pix border around each probe
    createInfo._initialHeight = (octPixelSize + 2) * numProbesPlane * numProbesHeight; // ditto
    createInfo._intialFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    createInfo._initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // hack since this is used in a pass before this one...

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams pipeParam{};
  pipeParam.device = rc->device();
  pipeParam.shader = "probe_conv_comp.spv";

  info._computeParams = pipeParam;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("IrradianceProbeConvolve",
    [this, numProbesPlane, numProbesHeight, octPixelSize](RenderExeParams exeParams) {
      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      auto extent = exeParams.rc->swapChainExtent();

      uint32_t numX = octPixelSize * numProbesPlane;
      numX = numX / 8;
      uint32_t numY = octPixelSize * numProbesPlane * numProbesHeight;
      numY = numY / 8;

      vkCmdDispatch(*exeParams.cmdBuffer, numX, numY + 1, 1);
    });
}

}