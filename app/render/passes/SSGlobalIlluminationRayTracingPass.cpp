#include "SSGlobalIlluminationRayTracingPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../VulkanExtensions.h"

namespace render {

SSGlobalIlluminationRayTracingPass::SSGlobalIlluminationRayTracingPass()
  : RenderPass()
{}

SSGlobalIlluminationRayTracingPass::~SSGlobalIlluminationRayTracingPass()
{}

void SSGlobalIlluminationRayTracingPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "SSGI";

  const int numSurfelsX = std::ceil(rc->swapChainExtent().width / 8.0);
  const int numSurfelsY = std::ceil(rc->swapChainExtent().height / 8.0);
  const int octSize = 16;

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
    usage._resourceName = "SSGITex";
    usage._access.set((std::size_t)Access::Read);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);

    ImageInitialCreateInfo createInfo{};

    createInfo._initialWidth = rc->swapChainExtent().width/2;
    createInfo._initialHeight = rc->swapChainExtent().height/2;
    createInfo._intialFormat = VK_FORMAT_R8G8B8A8_UNORM;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  RayTracingPipelineCreateParams param{};
  param.device = rc->device();
  param.rc = rc;
  param.raygenShader = "ssgi_rgen.spv";
  param.missShader = "ssgi_rmiss.spv";
  param.closestHitShader = "ssgi_rchit.spv";
  param.maxRecursionDepth = 2;
  info._rtParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("SSGI",
    [this, numSurfelsX, numSurfelsY, octSize](RenderExeParams exeParams) {
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
        exeParams.rc->swapChainExtent().width/2,
        exeParams.rc->swapChainExtent().height/2,
        1);
    });
}

}