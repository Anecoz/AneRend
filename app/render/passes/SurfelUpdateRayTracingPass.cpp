#include "SurfelUpdateRayTracingPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../VulkanExtensions.h"
#include "../RenderResource.h"

#include "SurfelCascadeInfo.h"

namespace render {

SurfelUpdateRayTracingPass::SurfelUpdateRayTracingPass()
  : RenderPass()
{}

SurfelUpdateRayTracingPass::~SurfelUpdateRayTracingPass()
{}

void SurfelUpdateRayTracingPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  for (int i = 0; i < 4; ++i) {
    Cascade casc{ surfel::surfPixSize[i], surfel::numRaysPerSurfel[i], surfel::sqrtNumRaysPerSurfel[i]};
    _cascades.emplace_back(std::move(casc));
    registerCascade(fgb, rc, i);
  }
}

void SurfelUpdateRayTracingPass::registerCascade(FrameGraphBuilder& fgb, RenderContext* rc, int cascade)
{
  std::string rpName = "SurfelUpdateRT" + std::to_string(cascade);
  RenderPassRegisterInfo info{};
  info._name = rpName;
  info._group = "SurfelUpdateRT";

  const int surfelPixSize = _cascades[cascade].surfelPixSize;
  //const int numRaysPerSurfel = _cascades[cascade].numRaysPerSurfel;
  const int sqrtNumRaysPerSurfel = _cascades[cascade].sqrtNumRaysPerSurfel;

  const int numSurfelsX = static_cast<int>(std::ceil(rc->swapChainExtent().width / surfelPixSize));
  const int numSurfelsY = static_cast<int>(std::ceil(rc->swapChainExtent().height / surfelPixSize));

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
    usage._resourceName = "Geometry0Image"; // normals in rgb channel
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SurfelDirTex" + std::to_string(cascade);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = numSurfelsX * sqrtNumRaysPerSurfel;
    createInfo._initialHeight = numSurfelsY * sqrtNumRaysPerSurfel;
    createInfo._intialFormat = VK_FORMAT_R8G8B8A8_UNORM;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SurfelIrrTex" + std::to_string(cascade);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = numSurfelsX * sqrtNumRaysPerSurfel;
    createInfo._initialHeight = numSurfelsY * sqrtNumRaysPerSurfel;
    createInfo._intialFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  RayTracingPipelineCreateParams param{};
  param.device = rc->device();
  param.rc = rc;
  param.raygenShader = "surfel_update_rgen.spv";
  param.missShader = "surfel_update_rmiss.spv";
  param.closestHitShader = "surfel_update_rchit.spv";
  param.maxRecursionDepth = 2;
  info._rtParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe(rpName,
    [this, numSurfelsX, numSurfelsY, sqrtNumRaysPerSurfel, cascade](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().screenspaceProbes) return;
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

      uint32_t cascadePc = (uint32_t)cascade;

      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        0,
        sizeof(uint32_t),
        &cascadePc);

      VkStridedDeviceAddressRegionKHR emptyRegion{};

      // Trace some rays
      vkext::vkCmdTraceRaysKHR(
        *exeParams.cmdBuffer,
        &exeParams.sbt->_rgenRegion,
        &exeParams.sbt->_missRegion,
        &exeParams.sbt->_chitRegion,
        &emptyRegion,
        numSurfelsX * sqrtNumRaysPerSurfel,
        numSurfelsY * sqrtNumRaysPerSurfel,
        1);
    });
}

}