#include "SurfelUpdateRayTracingPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../VulkanExtensions.h"

namespace render {

SurfelUpdateRayTracingPass::SurfelUpdateRayTracingPass()
  : RenderPass()
{}

SurfelUpdateRayTracingPass::~SurfelUpdateRayTracingPass()
{}

void SurfelUpdateRayTracingPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "SurfelUpdateRT";

  const int numSurfelsX = std::ceil(rc->swapChainExtent().width / 32.0);
  const int numSurfelsY = std::ceil(rc->swapChainExtent().height / 32.0);
  const int octSize = 8;

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
    usage._resourceName = "RTShadowImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SurfelTex";
    usage._access.set((std::size_t)Access::Read);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);

    ImageInitialCreateInfo createInfo{};
    // 8x8 probe resolution
    createInfo._initialWidth = numSurfelsX * octSize + numSurfelsX * 2; // 1 pix border around each cell
    createInfo._initialHeight = numSurfelsY * octSize + numSurfelsY * 2; // ditto
    createInfo._intialFormat = VK_FORMAT_R8G8B8A8_UNORM;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  /* {
    ResourceUsage usage{};
    usage._resourceName = "SurfelOctBuf";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);

    BufferInitialCreateInfo createInfo{};
    createInfo._initialSize = 8 * 8 * numSurfelsX * numSurfelsY * sizeof(glm::vec4);

    usage._bufferCreateInfo = createInfo;

    usage._type = Type::SSBO;
    info._resourceUsages.emplace_back(std::move(usage));
  }*/

  // Create a bunch of (one for each surfel) 8x8 oct images that will be put to a bindless array  
  /*for (int x = 0; x < numSurfelsX; ++x) {
    for (int y = 0; y < numSurfelsY; ++y) {
      ResourceUsage usage{};
      usage._resourceName = "SurfelTex_" + std::to_string(x) + "_" + std::to_string(y);
      usage._access.set((std::size_t)Access::Read);
      usage._access.set((std::size_t)Access::Write);
      usage._stage.set((std::size_t)Stage::RayTrace);
      usage._bindless = true;

      ImageInitialCreateInfo createInfo{};
      // 8x8 oct resolution
      createInfo._initialHeight = 8;
      createInfo._initialWidth = 8;
      createInfo._intialFormat = VK_FORMAT_R8G8B8A8_UNORM;

      usage._imageCreateInfo = createInfo;

      usage._type = Type::ImageStorage;
      info._resourceUsages.emplace_back(std::move(usage));
    }
  }*/

  RayTracingPipelineCreateParams param{};
  param.device = rc->device();
  param.rc = rc;
  param.raygenShader = "surfel_update_rgen.spv";
  param.missShader = "surfel_update_rmiss.spv";
  param.closestHitShader = "surfel_update_rchit.spv";
  param.maxRecursionDepth = 2;
  info._rtParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("SurfelUpdateRT",
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

      uint32_t numRaysPerSurfel = octSize * octSize;

      // Trace some rays
      vkext::vkCmdTraceRaysKHR(
        *exeParams.cmdBuffer,
        &exeParams.sbt->_rgenRegion,
        &exeParams.sbt->_missRegion,
        &exeParams.sbt->_chitRegion,
        &emptyRegion,
        numSurfelsX,
        numSurfelsY,
        numRaysPerSurfel);
      });
}

}