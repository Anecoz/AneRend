#include "IrradianceProbeConvolvePass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../ImageHelpers.h"

namespace render {

void clearConvTex(RenderContext* renderContext, VkImage& image)
{
  auto cmdBuf = renderContext->beginSingleTimeCommands();

  imageutil::transitionImageLayout(
    cmdBuf,
    image,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkClearColorValue clearVal{};
  clearVal.float32[0] = 0.0;
  clearVal.float32[1] = 0.0;
  clearVal.float32[2] = 0.0;
  clearVal.float32[3] = 1.0;

  VkImageSubresourceRange subRange{};
  subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subRange.layerCount = 1;
  subRange.levelCount = 1;

  vkCmdClearColorImage(
    cmdBuf,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    &clearVal,
    1,
    &subRange);

  imageutil::transitionImageLayout(
    cmdBuf,
    image,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  renderContext->endSingleTimeCommands(cmdBuf);
}

IrradianceProbeConvolvePass::IrradianceProbeConvolvePass()
  : RenderPass()
{}

IrradianceProbeConvolvePass::~IrradianceProbeConvolvePass()
{}

void IrradianceProbeConvolvePass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "IrradianceProbeConvolve";
  info._group = "IrradianceProbe";

  const int octPixelSize = 8;
  const int numProbesPlane = static_cast<int>(rc->getNumIrradianceProbesXZ());
  const int numProbesHeight = static_cast<int>(rc->getNumIrradianceProbesY());

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
    createInfo._initialDataCb = clearConvTex;

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
      if (!exeParams.rc->getRenderOptions().raytracingEnabled) return;
      if (!exeParams.rc->getRenderOptions().ddgiEnabled) return;

      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      // Choose a layer
      uint32_t probeLayer = (uint32_t)exeParams.rc->blackboardValueInt("ProbeLayer");

      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        0,
        sizeof(uint32_t),
        &probeLayer);

      auto extent = exeParams.rc->swapChainExtent();

      uint32_t numX = octPixelSize * numProbesPlane;
      numX = numX / 8;
      uint32_t numY = octPixelSize * numProbesPlane;
      numY = numY / 8;

      vkCmdDispatch(*exeParams.cmdBuffer, numX, numY + 1, 1);
    });
}

}