#include "ShadowRayTracingPass.h"

#include "../FrameGraphBuilder.h"
#include "../VulkanExtensions.h"

namespace render {

ShadowRayTracingPass::ShadowRayTracingPass()
  : RenderPass()
{}

ShadowRayTracingPass::~ShadowRayTracingPass()
{}

void ShadowRayTracingPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "ShadowRT";

  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::SampledDepthTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "RTShadowImage";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::ImageStorage;

    ImageInitialCreateInfo createInfo{};
    createInfo._initialHeight = rc->swapChainExtent().height;
    createInfo._initialWidth = rc->swapChainExtent().width;
    createInfo._intialFormat = VK_FORMAT_R32_SFLOAT;

    usage._imageCreateInfo = createInfo;

    info._resourceUsages.emplace_back(std::move(usage));
  }

  RayTracingPipelineCreateParams param{};
  param.device = rc->device();
  param.rc = rc;
  param.raygenShader = "shadow_rgen.spv";
  param.missShader = "shadow_rmiss.spv";
  param.closestHitShader = "shadow_rchit.spv";
  info._rtParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("ShadowRT",
    [this](RenderExeParams exeParams) {
      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, *exeParams.pipeline);

      // Descriptor set in set 1
      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      VkStridedDeviceAddressRegionKHR emptyRegion{};

      // Trace some rays
      vkext::vkCmdTraceRaysKHR(
        *exeParams.cmdBuffer,
        &exeParams.sbt->_rgenRegion,
        &exeParams.sbt->_missRegion,
        &exeParams.sbt->_chitRegion,
        &emptyRegion,
        exeParams.rc->swapChainExtent().width,
        exeParams.rc->swapChainExtent().height,
        1);
    });
}

}