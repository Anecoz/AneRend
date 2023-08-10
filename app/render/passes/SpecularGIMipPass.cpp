#include "SpecularGIMipPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"

namespace render {

SpecularGIMipPass::SpecularGIMipPass()
  : RenderPass()
{}

SpecularGIMipPass::~SpecularGIMipPass()
{}

void SpecularGIMipPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  uint32_t width = rc->swapChainExtent().width / 2;
  uint32_t height = rc->swapChainExtent().height / 2;
  uint32_t mipLevels = 4;// static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

  for (int i = 0; i < mipLevels - 1; ++i) {
    RenderPassRegisterInfo info{};
    info._name = "SpecularGIMipGen" + std::to_string(i);

    {
      ResourceUsage usage{};
      usage._resourceName = "ReflectTex";
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._allMips = false;
      usage._mip = i;
      usage._type = Type::SampledTexture;
      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "GeometryDepthImage";
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._type = Type::SampledDepthTexture;
      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "ReflectTex";
      usage._access.set((std::size_t)Access::Write);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._allMips = false;
      usage._mip = i+1;
      usage._type = Type::ImageStorage;
      info._resourceUsages.emplace_back(std::move(usage));
    }

    ComputePipelineCreateParams pipeParam{};
    pipeParam.device = rc->device();
    pipeParam.shader = "bilateral_filter_comp.spv";

    info._computeParams = pipeParam;

    fgb.registerRenderPass(std::move(info));

    fgb.registerRenderPassExe("SpecularGIMipGen" + std::to_string(i),
      [this, i](RenderExeParams exeParams) {
        if (!exeParams.rc->getRenderOptions().raytracingEnabled) return;
        if (!exeParams.rc->getRenderOptions().specularGiEnabled) return;

        // Bind pipeline
        vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

        vkCmdBindDescriptorSets(
          *exeParams.cmdBuffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          *exeParams.pipelineLayout,
          1, 1, &(*exeParams.descriptorSets)[0],
          0, nullptr);

        uint32_t width = exeParams.rc->swapChainExtent().width / (2 + (i+1)*2);
        uint32_t height = exeParams.rc->swapChainExtent().height / (2 + (i + 1) * 2);

        vkCmdDispatch(*exeParams.cmdBuffer, width, height, 1);
      });
  }
}

}