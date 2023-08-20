#include "DeferredLightingRenderPass.h"

#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../FrameGraphBuilder.h"
#include "../../util/GraphicsUtils.h"
#include "../ImageHelpers.h"
#include "../BufferHelpers.h"

namespace render {

namespace {

void loadLutTex(RenderContext* renderContext, VkImage& image)
{
  auto texData = imageutil::loadTex(ASSET_PATH + std::string("/images/brdf_lut.png"));

  // Create a staging buffer on CPU side first
  AllocatedBuffer stagingBuffer;
  std::size_t dataSize = texData.data.size();

  auto vmaAllocator = renderContext->vmaAllocator();

  bufferutil::createBuffer(vmaAllocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  glm::uint8_t* mappedData;
  vmaMapMemory(vmaAllocator, stagingBuffer._allocation, &(void*)mappedData);
  std::memcpy(mappedData, texData.data.data(), texData.data.size());
  vmaUnmapMemory(vmaAllocator, stagingBuffer._allocation);

  auto cmdBuffer = renderContext->beginSingleTimeCommands();

  // Transition image to dst optimal
  imageutil::transitionImageLayout(
    cmdBuffer,
    image,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkExtent3D extent{};
  extent.depth = 1;
  extent.height = texData.height;
  extent.width = texData.width;

  VkOffset3D offset{};

  VkBufferImageCopy imCopy{};
  imCopy.imageExtent = extent;
  imCopy.imageOffset = offset;
  imCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imCopy.imageSubresource.layerCount = 1;

  vkCmdCopyBufferToImage(
    cmdBuffer,
    stagingBuffer._buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &imCopy);

  // Transition to shader read
  imageutil::transitionImageLayout(
    cmdBuffer,
    image,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  renderContext->endSingleTimeCommands(cmdBuffer);

  vmaDestroyBuffer(vmaAllocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

}


DeferredLightingRenderPass::DeferredLightingRenderPass()
  : RenderPass()
{}

DeferredLightingRenderPass::~DeferredLightingRenderPass()
{}

void DeferredLightingRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "DeferredPbrLight";

  std::vector<ResourceUsage> resourceUsages;
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry0Image";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._noSamplerFiltering = true;
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry1Image";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry2Image";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledDepthTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ShadowMap";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledDepthTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOBlurImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "RTShadowImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  /*{
    ResourceUsage usage{};
    usage._resourceName = "IrradianceProbeSSBO";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;
    resourceUsages.emplace_back(std::move(usage));
  }*/
  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImage";
    usage._access.set((std::size_t)Access::Read);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialHeight = rc->swapChainExtent().height;
    createInfo._initialWidth = rc->swapChainExtent().width;
    createInfo._intialFormat = VK_FORMAT_R8G8B8A8_UNORM;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    resourceUsages.emplace_back(std::move(usage));
  }
  /* {
    ResourceUsage usage{};
    usage._resourceName = "SurfelConvTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._samplerClamp = true;
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }*/
  /* {
    ResourceUsage usage{};
    usage._resourceName = "SSGIBlurTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    //usage._samplerClamp = true;
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }*/
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysConvTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._samplerClampToEdge = true;
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ReflectTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledTexture;
    usage._samplerClampToBorder = true;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SpecularBRDFLutTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledTexture;

    ImageInitialCreateInfo createInfo{};
    createInfo._initialHeight = 400;
    createInfo._initialWidth = 400;
    createInfo._intialFormat = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo._initialDataCb = loadLutTex;

    usage._imageCreateInfo = createInfo;
    usage._samplerClampToEdge = true;

    resourceUsages.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsages);

  ComputePipelineCreateParams param{};
  param.device = rc->device();
  //param.pipelineLayout = _pipelineLayout;
  param.shader = "deferred_tiled_comp.spv";
  info._computeParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("DeferredPbrLight",
    [this](RenderExeParams exeParams) {
      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      auto extent = exeParams.rc->swapChainExtent();

      uint32_t numX = extent.width / 32;
      uint32_t numY = extent.height / 32;

      std::uint32_t pushData[2];
      pushData[0] = extent.width;
      pushData[1] = extent.height;

      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        0,
        2 * sizeof(uint32_t),
        pushData);
        
      vkCmdDispatch(*exeParams.cmdBuffer, numX, numY + 1, 1);
    });
}

}