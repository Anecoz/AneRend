#include "SurfelSHPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"

#include "SurfelCascadeInfo.h"

namespace render
{

SurfelSHPass::SurfelSHPass()
  : RenderPass()
{}

SurfelSHPass::~SurfelSHPass()
{}

void SurfelSHPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  for (int i = 0; i < 4; ++i) {
    std::string rpName = "SurfelSH" + std::to_string(i);
    RenderPassRegisterInfo info{};
    info._name = rpName;
    info._group = "SurfelSH";

    const int numSurfelsX = static_cast<int>(std::ceil(rc->swapChainExtent().width / surfel::surfPixSize[i]));
    const int numSurfelsY = static_cast<int>(std::ceil(rc->swapChainExtent().height / surfel::surfPixSize[i]));

    {
      ResourceUsage usage{};
      usage._resourceName = "SurfelDirTex" + std::to_string(i);
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._type = Type::ImageStorage;
      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "SurfelIrrTex" + std::to_string(i);
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._type = Type::ImageStorage;
      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "SurfelSH" + std::to_string(i);
      usage._access.set((std::size_t)Access::Write);
      usage._stage.set((std::size_t)Stage::Compute);

      ImageInitialCreateInfo createInfo{};
      createInfo._initialWidth = 3 * numSurfelsX;
      createInfo._initialHeight = 3 * numSurfelsY;
      createInfo._intialFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

      usage._imageCreateInfo = createInfo;

      usage._type = Type::ImageStorage;
      info._resourceUsages.emplace_back(std::move(usage));
    }

    ComputePipelineCreateParams pipeParam{};
    pipeParam.device = rc->device();
    pipeParam.shader = "surfel_sh_comp.spv";

    info._computeParams = pipeParam;

    fgb.registerRenderPass(std::move(info));

    fgb.registerRenderPassExe(rpName,
      [this, numSurfelsX, numSurfelsY, i](RenderExeParams exeParams) {
        if (!exeParams.rc->getRenderOptions().raytracingEnabled) return;
        if (!exeParams.rc->getRenderOptions().screenspaceProbes) return;

        // Bind pipeline
        vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

        vkCmdBindDescriptorSets(
          *exeParams.cmdBuffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          *exeParams.pipelineLayout,
          1, 1, &(*exeParams.descriptorSets)[0],
          0, nullptr);

        uint32_t cascadePc = (uint32_t)i;

        vkCmdPushConstants(
          *exeParams.cmdBuffer,
          *exeParams.pipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
          0,
          sizeof(uint32_t),
          &cascadePc);

        const int localGroupSize = 16;
        //const int sqrtNumRaysPerSurfel = surfel::sqrtNumRaysPerSurfel[i];
        uint32_t numX = numSurfelsX / localGroupSize;
        uint32_t numY = numSurfelsY / localGroupSize;

        // Z group size is 9 in compute shader
        vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, numY + 1, 9);
      });
  }
}

}