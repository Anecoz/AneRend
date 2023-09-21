#include "BloomRenderPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"

namespace render {

BloomRenderPass::BloomRenderPass()
  : RenderPass()
{}

BloomRenderPass::~BloomRenderPass()
{}

void BloomRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Does one downsample and thresholds the input image
  prefilterPass(fgb, rc);

  // Downsamples into a mip chain
  downsamplePass(fgb, rc);

  // Continuously upsamples and combines the images
  upsamplePass(fgb, rc);

  // Composites original scene image with the completed bloom image, and tonemaps
  compositePass(fgb, rc);
}

void BloomRenderPass::prefilterPass(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // The first step just does a threshold filter + downsample
  RenderPassRegisterInfo info{};
  info._name = "BloomPreFilter";
  info._group = "Bloom";

  {
    ResourceUsage usage{};
    usage._resourceName = "BloomDsTex";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialHeight = rc->swapChainExtent().height/2;
    createInfo._initialWidth = rc->swapChainExtent().width/2;
    createInfo._intialFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    createInfo._mipLevels = 7;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams param{};
  param.device = rc->device();
  param.shader = "bloom_prefilter_comp.spv";
  info._computeParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("BloomPreFilter",
    [this](RenderExeParams exeParams) {
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      uint32_t localSize = 8;
      uint32_t numY = exeParams.rc->swapChainExtent().height / 2 / localSize;
      uint32_t numX = exeParams.rc->swapChainExtent().width / 2 / localSize;

      vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, numY + 1, 1);
    });
}

void BloomRenderPass::downsamplePass(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Do a downsample for each mip, using previous mip as input
  for (int mip = 1; mip < 7; ++mip) {
    RenderPassRegisterInfo info{};
    std::string name = "BloomDownsample" + std::to_string(mip);
    info._name = name;
    info._group = "Bloom";

    {
      ResourceUsage usage{};
      usage._resourceName = "BloomDsTex";
      usage._access.set((std::size_t)Access::Write);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._allMips = false;
      usage._mip = mip;
      usage._minLod = float(mip);
      usage._maxLod = float(mip);
      usage._type = Type::ImageStorage;
      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "BloomDsTex";
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._allMips = false;
      usage._minLod = float(mip - 1);
      usage._maxLod = float(mip - 1);
      usage._mip = mip - 1;
      usage._samplerClampToEdge = true;
      usage._type = Type::SampledTexture;
      info._resourceUsages.emplace_back(std::move(usage));
    }

    ComputePipelineCreateParams param{};
    param.device = rc->device();
    param.shader = "bloom_downsample_comp.spv";
    info._computeParams = param;

    fgb.registerRenderPass(std::move(info));

    fgb.registerRenderPassExe(name,
      [this, mip](RenderExeParams exeParams) {
        vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

        vkCmdBindDescriptorSets(
          *exeParams.cmdBuffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          *exeParams.pipelineLayout,
          1, 1, &(*exeParams.descriptorSets)[0],
          0, nullptr);

        uint32_t localSize = 8;
        uint32_t initialHeight = exeParams.rc->swapChainExtent().height / 2;
        uint32_t initialWidth = exeParams.rc->swapChainExtent().width / 2;

        uint32_t height = initialHeight / static_cast<uint32_t>(std::pow(2, mip));
        uint32_t width = initialWidth / static_cast<uint32_t>(std::pow(2, mip));

        uint32_t numY = height / localSize;
        uint32_t numX = width / localSize;

        vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, numY + 1, 1);
      });
  }
}

void BloomRenderPass::upsamplePass(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Initial pass to create the us image and get the first mip written
  {
    RenderPassRegisterInfo info{};
    std::string name = "BloomUpsample5";
    info._name = name;
    info._group = "Bloom";

    {
      ResourceUsage usage{};
      usage._resourceName = "BloomUsTex";
      usage._access.set((std::size_t)Access::Write);
      usage._stage.set((std::size_t)Stage::Compute);

      ImageInitialCreateInfo createInfo{};
      createInfo._initialHeight = rc->swapChainExtent().height / 2;
      createInfo._initialWidth = rc->swapChainExtent().width / 2;
      createInfo._intialFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
      createInfo._mipLevels = 6; // one less

      usage._imageCreateInfo = createInfo;
      usage._allMips = false;
      usage._mip = 5;

      usage._type = Type::ImageStorage;
      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "BloomDsTex";
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._allMips = false;
      usage._mip = 5;
      usage._samplerClampToEdge = true;
      usage._type = Type::SampledTexture;
      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "BloomDsTex";
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._allMips = false;
      usage._mip = 6;
      usage._samplerClampToEdge = true;
      usage._type = Type::SampledTexture;
      info._resourceUsages.emplace_back(std::move(usage));
    }

    ComputePipelineCreateParams param{};
    param.device = rc->device();
    param.shader = "bloom_upsample_comp.spv";
    info._computeParams = param;

    fgb.registerRenderPass(std::move(info));

    fgb.registerRenderPassExe(name,
      [this](RenderExeParams exeParams) {
        vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

        vkCmdBindDescriptorSets(
          *exeParams.cmdBuffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          *exeParams.pipelineLayout,
          1, 1, &(*exeParams.descriptorSets)[0],
          0, nullptr);

        uint32_t localSize = 8;
        uint32_t initialHeight = exeParams.rc->swapChainExtent().height / 2;
        uint32_t initialWidth = exeParams.rc->swapChainExtent().width / 2;

        uint32_t height = initialHeight / static_cast<uint32_t>(std::pow(2, 5));
        uint32_t width = initialWidth / static_cast<uint32_t>(std::pow(2, 5));

        uint32_t numY = height / localSize;
        uint32_t numX = width / localSize;

        vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, numY + 1, 1);
      });
  }

  // Rest of the passes
  for (int mip = 4; mip >= 0; --mip) {
    RenderPassRegisterInfo info{};
    std::string name = "BloomUpsample" + std::to_string(mip);
    info._name = name;
    info._group = "Bloom";

    {
      ResourceUsage usage{};
      usage._resourceName = "BloomUsTex";
      usage._access.set((std::size_t)Access::Write);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._allMips = false;
      usage._mip = mip;

      usage._type = Type::ImageStorage;
      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "BloomDsTex";
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._allMips = false;
      usage._mip = mip;
      usage._samplerClampToEdge = true;
      usage._type = Type::SampledTexture;
      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "BloomUsTex";
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._allMips = false;
      usage._mip = mip + 1;
      usage._samplerClampToEdge = true;
      usage._type = Type::SampledTexture;
      info._resourceUsages.emplace_back(std::move(usage));
    }

    ComputePipelineCreateParams param{};
    param.device = rc->device();
    param.shader = "bloom_upsample_comp.spv";
    info._computeParams = param;

    fgb.registerRenderPass(std::move(info));

    fgb.registerRenderPassExe(name,
      [this, mip](RenderExeParams exeParams) {
        vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

        vkCmdBindDescriptorSets(
          *exeParams.cmdBuffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          *exeParams.pipelineLayout,
          1, 1, &(*exeParams.descriptorSets)[0],
          0, nullptr);

        uint32_t localSize = 8;
        uint32_t initialHeight = exeParams.rc->swapChainExtent().height / 2;
        uint32_t initialWidth = exeParams.rc->swapChainExtent().width / 2;

        uint32_t height = initialHeight / static_cast<uint32_t>(std::pow(2, mip));
        uint32_t width = initialWidth / static_cast<uint32_t>(std::pow(2, mip));

        uint32_t numY = height / localSize;
        uint32_t numX = width / localSize;

        vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, numY + 1, 1);
      });
  }  
}

void BloomRenderPass::compositePass(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "BloomComposite";
  info._group = "Bloom";

  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImageBloom";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialHeight = rc->swapChainExtent().height;
    createInfo._initialWidth = rc->swapChainExtent().width;
    createInfo._intialFormat = rc->getHDRFormat();

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "BloomUsTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._samplerClampToEdge = true;
    usage._allMips = false;
    usage._mip = 0;
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._samplerClampToEdge = true;
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "AvgLumSSBO";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams param{};
  param.device = rc->device();
  param.shader = "bloom_composite_comp.spv";
  info._computeParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("BloomComposite",
    [this](RenderExeParams exeParams) {
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      uint32_t localSize = 8;
      uint32_t numY = exeParams.rc->swapChainExtent().height / localSize;
      uint32_t numX = exeParams.rc->swapChainExtent().width / localSize;

      vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, numY + 1, 1);
    });
}

}