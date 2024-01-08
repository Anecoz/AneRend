#include "SSAORenderPass.h"

#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../FrameGraphBuilder.h"
#include "../ImageHelpers.h"
#include "../BufferHelpers.h"
#include "../../util/GraphicsUtils.h"

#include <random>

namespace render {

namespace {

float lerp(float a, float b, float f)
{
  return a + f * (b - a);
}

void fillSampleUBO(RenderContext* renderContext, AllocatedBuffer& ubo)
{
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
  std::default_random_engine generator;
  std::vector<glm::vec4> ssaoKernel;
  for (unsigned int i = 0; i < 64; ++i) {
    glm::vec4 sample(
      randomFloats(generator) * 2.0f - 1.0f,
      randomFloats(generator) * 2.0f - 1.0f,
      randomFloats(generator),
      0.0f
    );
    sample = glm::normalize(sample);
    sample *= randomFloats(generator);
    float scale = (float)i / 64.0f;
    scale = lerp(0.1f, 1.0f, scale * scale);
    sample *= scale;

    ssaoKernel.push_back(sample);
  }

  // Create a staging buffer on CPU side first
  AllocatedBuffer stagingBuffer;
  std::size_t dataSize = 64 * sizeof(glm::vec4);

  auto allocator = renderContext->vmaAllocator();
  bufferutil::createBuffer(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  void* data;
  glm::vec4* mappedData;
  vmaMapMemory(allocator, stagingBuffer._allocation, &data);
  mappedData = (glm::vec4*)data;

  for (std::size_t i = 0; i < ssaoKernel.size(); ++i) {
    mappedData[i] = ssaoKernel[i];
  }

  auto cmdBuf = renderContext->beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = dataSize;

  vkCmdCopyBuffer(
    cmdBuf,
    stagingBuffer._buffer,
    ubo._buffer,
    1, &copyRegion);

  renderContext->endSingleTimeCommands(cmdBuf);

  vmaUnmapMemory(allocator, stagingBuffer._allocation);
  vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);

  /*void* data;
  auto allocator = renderContext->vmaAllocator();
  vmaMapMemory(allocator, ubo._allocation, &data);
  memcpy(data, ssaoKernel.data(), sizeof(gpu::GPUSSAOSampleUbo));
  vmaUnmapMemory(allocator, ubo._allocation);*/
}
  
void fillNoiseTex(RenderContext* renderContext, VkImage& image)
{
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
  std::default_random_engine generator;

  std::vector<glm::vec4> ssaoNoise;
  for (unsigned int i = 0; i < 16; i++) {
    glm::vec4 noise(
      randomFloats(generator) * 2.0 - 1.0,
      randomFloats(generator) * 2.0 - 1.0,
      0.0f,
      0.0f);
    ssaoNoise.push_back(noise);
  }

  // Create a staging buffer on CPU side first
  AllocatedBuffer stagingBuffer;
  std::size_t dataSize = 16 * sizeof(glm::vec4);

  auto allocator = renderContext->vmaAllocator();
  bufferutil::createBuffer(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  void* data;
  glm::vec4* mappedData;
  vmaMapMemory(allocator, stagingBuffer._allocation, &data);
  mappedData = (glm::vec4*)data;

  for (std::size_t i = 0; i < ssaoNoise.size(); ++i) {
    mappedData[i] = ssaoNoise[i];
  }

  vmaUnmapMemory(allocator, stagingBuffer._allocation);

  auto cmdBuffer = renderContext->beginSingleTimeCommands();

  // Transition image to dst optimal
  imageutil::transitionImageLayout(
    cmdBuffer,
    image,
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_IMAGE_LAYOUT_UNDEFINED, 
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkExtent3D extent{};
  extent.depth = 1;
  extent.height = 4;
  extent.width = 4;

  VkOffset3D offset{};
  offset.x = 0;
  offset.y = 0;
  offset.z = 0;

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
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  renderContext->endSingleTimeCommands(cmdBuffer);

  vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

}

SSAORenderPass::SSAORenderPass()
  : RenderPass()
{}

SSAORenderPass::~SSAORenderPass()
{}

void SSAORenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "SSAO";
  info._group = "SSAO";

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
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledDepthTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAONoiseImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialHeight = 4;
    createInfo._initialWidth = 4;
    createInfo._intialFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    createInfo._initialDataCb = fillNoiseTex;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOSampleUbo";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Transfer);

    BufferInitialCreateInfo createInfo{};
    //createInfo._hostWritable = true;
    createInfo._initialSize = sizeof(gpu::GPUSSAOSampleUbo);
    createInfo._initialDataCb = fillSampleUBO;

    usage._bufferCreateInfo = createInfo;

    usage._type = Type::UBO;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOImage";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialHeight = rc->swapChainExtent().height;
    createInfo._initialWidth = rc->swapChainExtent().width;
    createInfo._intialFormat = VK_FORMAT_R32_SFLOAT;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    resourceUsages.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsages);

  ComputePipelineCreateParams param{};
  param.device = rc->device();
  param.shader = "ssao_comp.spv";
  info._computeParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("SSAO",
    [this](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().ssao) return;

      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      uint32_t localSize = 16;
      uint32_t numY = exeParams.rc->swapChainExtent().height / localSize;
      uint32_t numX = exeParams.rc->swapChainExtent().width / localSize;

      vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, numY + 1, 1);
    });
}

}