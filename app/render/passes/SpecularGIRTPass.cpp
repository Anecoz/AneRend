#include "SpecularGIRTPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../VulkanExtensions.h"

namespace render {

SpecularGIRTPass::SpecularGIRTPass()
  : RenderPass()
{}

SpecularGIRTPass::~SpecularGIRTPass()
{}

void SpecularGIRTPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "SpecularGIRT";
  info._group = "SpecularGI";

  // This texture will contain perfect mirror reflections radiance values in a screen-space fashion.
  // the alpha channel will contain distance between the objects
  {
    uint32_t width = rc->swapChainExtent().width;
    uint32_t height = rc->swapChainExtent().height;
    uint32_t mipLevels = 5;// static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    ResourceUsage usage{};
    usage._resourceName = "ReflectTex";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._allMips = false;
    usage._mip = 0;

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = width;
    createInfo._initialHeight = height;
    createInfo._intialFormat = VK_FORMAT_R16G16B16A16_SFLOAT; // Needs to be HDR
    createInfo._mipLevels = mipLevels;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
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
    usage._resourceName = "Geometry0Image";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::SampledTexture;
    usage._noSamplerFiltering = true;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysConvTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry1Image";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SpecularBRDFLutTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  RayTracingPipelineCreateParams param{};
  param.device = rc->device();
  param.rc = rc;
  param.raygenShader = "mirror_reflections_rgen.spv";
  param.missShader = "mirror_reflections_rmiss.spv";
  param.closestHitShader = "mirror_reflections_rchit.spv";
  param.maxRecursionDepth = 2;
  info._rtParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("SpecularGIRT",
    [this](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().raytracingEnabled) return;
      if (!exeParams.rc->getRenderOptions().specularGiEnabled) return;

      double elapsedTime = exeParams.rc->getElapsedTime();
      if (elapsedTime - _lastRayTraceTime < (1.0 / _traceRate)) {
        return;
      }

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

      _lastRayTraceTime = elapsedTime;
    });
}

}