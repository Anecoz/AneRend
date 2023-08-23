#include "LightShadowRayTracingPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../VulkanExtensions.h"

namespace render
{

LightShadowRayTracingPass::LightShadowRayTracingPass()
  : RenderPass()
{}

LightShadowRayTracingPass::~LightShadowRayTracingPass()
{}

void LightShadowRayTracingPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "LightShadowRT";

  const int numRaysPerLight = 1024;
  const int sqrtNumRays = std::sqrt(numRaysPerLight);
  const int maxNumLights = 32 * 32;

  // Texture to capture the rays irradiance information, which will later be integrated per probe oct direction
  {
    ResourceUsage usage{};
    usage._resourceName = "LightShadowDistTex";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = sqrtNumRays * maxNumLights;
    createInfo._initialHeight = sqrtNumRays;
    createInfo._intialFormat = VK_FORMAT_R32_SFLOAT;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  // Captures the direction of each ray, corresponding to the irradiance
  {
    ResourceUsage usage{};
    usage._resourceName = "LightShadowDirTex";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = sqrtNumRays * maxNumLights;
    createInfo._initialHeight = sqrtNumRays;
    createInfo._intialFormat = VK_FORMAT_R8G8B8A8_UNORM;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  RayTracingPipelineCreateParams param{};
  param.device = rc->device();
  param.rc = rc;
  param.raygenShader = "light_shadow_rgen.spv";
  param.missShader = "light_shadow_rmiss.spv";
  param.closestHitShader = "light_shadow_rchit.spv";
  param.maxRecursionDepth = 1;
  info._rtParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("LightShadowRT",
    [this, maxNumLights, sqrtNumRays](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().raytracingEnabled) return;

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
        sqrtNumRays,
        sqrtNumRays,
        maxNumLights);
    });
}

}