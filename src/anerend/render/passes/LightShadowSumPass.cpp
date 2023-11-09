#include "LightShadowSumPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"

namespace render
{

LightShadowSumPass::LightShadowSumPass()
  : RenderPass()
{}

LightShadowSumPass::~LightShadowSumPass()
{}

void LightShadowSumPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "LightShadowSum";

  const int octPixelSize = 8;
  const int maxNumLights = 32 * 32;

  {
    ResourceUsage usage{};
    usage._resourceName = "LightShadowDistTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "LightShadowDirTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "LightShadowMeanTex";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = (octPixelSize + 2) * maxNumLights; // 1 pix border around each light
    createInfo._initialHeight = octPixelSize;
    createInfo._intialFormat = VK_FORMAT_R32G32_SFLOAT;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams pipeParam{};
  pipeParam.device = rc->device();
  pipeParam.shader = "light_shadow_sum_comp.spv";

  info._computeParams = pipeParam;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("LightShadowSum",
    [this, maxNumLights, octPixelSize](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().raytracingEnabled) return;

      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      uint32_t numX = octPixelSize;
      numX = numX / 8;
      uint32_t numY = octPixelSize;
      numY = numY / 8;
      uint32_t numZ = maxNumLights;
      numZ = numZ / 8;

      vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, numY + 1, numZ + 1);
    });
}

}