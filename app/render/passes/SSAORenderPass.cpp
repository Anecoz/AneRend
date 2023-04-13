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

void fillSampleUBO(RenderContext* renderContext, AllocatedBuffer ubo)
{
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
  std::default_random_engine generator;
  std::vector<glm::vec3> ssaoKernel;
  for (unsigned int i = 0; i < 64; ++i) {
    glm::vec4 sample(
      randomFloats(generator) * 2.0 - 1.0,
      randomFloats(generator) * 2.0 - 1.0,
      randomFloats(generator),
      0.0
    );
    sample = glm::normalize(sample);
    sample *= randomFloats(generator);
    float scale = (float)i / 64.0;
    scale = lerp(0.1f, 1.0f, scale * scale);
    sample *= scale;

    ssaoKernel.push_back(sample);
  }

  void* data;
  auto allocator = renderContext->vmaAllocator();
  vmaMapMemory(allocator, ubo._allocation, &data);
  memcpy(data, ssaoKernel.data(), sizeof(gpu::GPUSSAOSampleUbo));
  vmaUnmapMemory(allocator, ubo._allocation);
}
  
void fillNoiseTex(RenderContext* renderContext, VkImage image)
{
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
  std::default_random_engine generator;

  std::vector<glm::vec3> ssaoNoise;
  for (unsigned int i = 0; i < 16; i++) {
    glm::vec3 noise(
      randomFloats(generator) * 2.0 - 1.0,
      randomFloats(generator) * 2.0 - 1.0,
      0.0f);
    ssaoNoise.push_back(noise);
  }

  // Create a staging buffer on CPU side first
  AllocatedBuffer stagingBuffer;
  std::size_t dataSize = 16 * sizeof(glm::vec3);

  auto allocator = renderContext->vmaAllocator();
  bufferutil::createBuffer(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  glm::vec3* mappedData;
  vmaMapMemory(allocator, stagingBuffer._allocation, &(void*)mappedData);

  for (std::size_t i = 0; i < ssaoNoise.size(); ++i) {
    mappedData[i] = ssaoNoise[i];
  }

  vmaUnmapMemory(allocator, stagingBuffer._allocation);

  auto cmdBuffer = renderContext->beginSingleTimeCommands();

  // Transition image to dst optimal
  imageutil::transitionImageLayout(
    cmdBuffer,
    image,
    VK_FORMAT_R16G16B16A16_SFLOAT,
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
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vmaUnmapMemory(allocator, stagingBuffer._allocation);

  renderContext->endSingleTimeCommands(cmdBuffer);

  vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

}

SSAORenderPass::SSAORenderPass()
  : RenderPass()
{}

SSAORenderPass::~SSAORenderPass()
{}

bool SSAORenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Create the output render target
  auto extent = renderContext->swapChainExtent();

  auto outputRes = new ImageRenderResource();
  outputRes->_format = VK_FORMAT_R32_SFLOAT;

  auto outputViewRes = new ImageViewRenderResource();
  outputViewRes->_format = VK_FORMAT_R32_SFLOAT;

  imageutil::createImage(
    extent.width,
    extent.height,
    outputRes->_format,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    outputRes->_image);

  outputViewRes->_view = imageutil::createImageView(renderContext->device(), outputRes->_image._image, outputRes->_format, VK_IMAGE_ASPECT_COLOR_BIT);

  vault->addResource("SSAOImage", std::unique_ptr<IRenderResource>(outputRes));
  vault->addResource("SSAOImageView", std::unique_ptr<IRenderResource>(outputViewRes));

  // Create the noise image
  auto noiseRes = new ImageRenderResource();
  noiseRes->_format = VK_FORMAT_R16G16B16A16_SFLOAT;

  auto noiseViewRes = new ImageViewRenderResource();
  noiseViewRes->_format = VK_FORMAT_R16G16B16A16_SFLOAT;

  imageutil::createImage(
    4,
    4,
    noiseViewRes->_format,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_SAMPLED_BIT,
    noiseRes->_image);

  noiseViewRes->_view = imageutil::createImageView(renderContext->device(), noiseRes->_image._image, noiseRes->_format, VK_IMAGE_ASPECT_COLOR_BIT);

  fillNoiseTex(renderContext, noiseRes->_image._image);

  vault->addResource("SSAONoiseImage", std::unique_ptr<IRenderResource>(noiseRes));
  vault->addResource("SSAONoiseImageView", std::unique_ptr<IRenderResource>(noiseViewRes));

  // Create UBO buffer for samples
  auto allocator = renderContext->vmaAllocator();

  bufferutil::createBuffer(
    allocator,
    sizeof(gpu::GPUSSAOSampleUbo),
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    _sampleBuffer);

  fillSampleUBO(renderContext, _sampleBuffer);

  // Desc layout for gbuffer samplers
  DescriptorBindInfo sampler0Info{};
  sampler0Info.binding = 0;
  sampler0Info.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  sampler0Info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorBindInfo depthSamplerInfo{};
  depthSamplerInfo.binding = 1;
  depthSamplerInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  depthSamplerInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorBindInfo noiseSamplerInfo{};
  noiseSamplerInfo.binding = 2;
  noiseSamplerInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  noiseSamplerInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorBindInfo sampleUBOInfo{};
  sampleUBOInfo.binding = 3;
  sampleUBOInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  sampleUBOInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(sampler0Info);
  descLayoutParam.bindInfos.emplace_back(depthSamplerInfo);
  descLayoutParam.bindInfos.emplace_back(noiseSamplerInfo);
  descLayoutParam.bindInfos.emplace_back(sampleUBOInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptor containing samplers
  auto im0 = (ImageViewRenderResource*)vault->getResource("Geometry0ImageView");
  auto depth = (ImageViewRenderResource*)vault->getResource("GeometryDepthImageView");

  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  SamplerCreateParams samplerParam{};
  samplerParam.renderContext = renderContext;
  _sampler0 = createSampler(samplerParam);
  _depthSampler = createSampler(samplerParam);

  samplerParam.addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  _noiseSampler = createSampler(samplerParam);

  sampler0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  sampler0Info.sampler = _sampler0;
  sampler0Info.view = im0->_view;
  descParam.bindInfos.emplace_back(sampler0Info);

  depthSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  depthSamplerInfo.sampler = _depthSampler;
  depthSamplerInfo.view = depth->_view;
  descParam.bindInfos.emplace_back(depthSamplerInfo);

  noiseSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  noiseSamplerInfo.sampler = _noiseSampler;
  noiseSamplerInfo.view = noiseViewRes->_view;
  descParam.bindInfos.emplace_back(noiseSamplerInfo);

  sampleUBOInfo.buffer = _sampleBuffer._buffer;
  descParam.bindInfos.emplace_back(sampleUBOInfo);

  descParam.multiBuffered = false;

  _descriptorSets = buildDescriptorSets(descParam);

  // Build a graphics pipeline
  GraphicsPipelineCreateParams param{};
  param.pipelineLayout = _pipelineLayout;
  param.device = renderContext->device();
  param.vertShader = "shadow_debug_vert.spv";
  param.fragShader = "ssao_frag.spv";
  param.depthTest = false;
  param.normalLoc = -1;
  param.colorLoc = -1;
  param.uvLoc = 1;
  param.colorFormat = VK_FORMAT_R32_SFLOAT;

  buildGraphicsPipeline(param);

  // Screen quad
  _screenQuad._vertices = graphicsutil::createScreenQuad(1.0f, 1.0f);

  _meshId = renderContext->registerMesh(_screenQuad._vertices, {});
  _renderableId = renderContext->registerRenderable(_meshId, glm::mat4(1.0f), glm::vec3(1.0f), 0.0f);
  renderContext->setRenderableVisible(_renderableId, false);

  return true;
}

void SSAORenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "SSAO";

  std::vector<ResourceUsage> resourceUsages;
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry0Image";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAONoiseImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOImage";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::ColorAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsages);
  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("SSAO",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra) {
      // Get resources
      auto outputImView = (ImageViewRenderResource*)vault->getResource("SSAOImageView");

      VkClearValue clearValue{};
      clearValue.color = { {1.0f, 1.0f, 1.0f, 1.0f} };
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = outputImView->_view;
      colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachmentInfo.clearValue = clearValue;

      VkRenderingInfoKHR renderInfo{};
      renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderInfo.renderArea.extent = renderContext->swapChainExtent();
      renderInfo.renderArea.offset = { 0, 0 };
      renderInfo.layerCount = 1;
      renderInfo.colorAttachmentCount = 1;
      renderInfo.pColorAttachments = &colorAttachmentInfo;
      renderInfo.pDepthAttachment = nullptr;

      vkCmdBeginRendering(*cmdBuffer, &renderInfo);

      // Viewport and scissor
      VkViewport viewport{};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(renderContext->swapChainExtent().width);
      viewport.height = static_cast<float>(renderContext->swapChainExtent().height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor{};
      scissor.offset = { 0, 0 };
      scissor.extent = renderContext->swapChainExtent();

      // Bind pipeline
      vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

      vkCmdBindDescriptorSets(
        *cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout,
        1, 1, &_descriptorSets[0],
        0, nullptr);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*cmdBuffer, 0, 1, &scissor);

      renderContext->drawMeshId(cmdBuffer, _meshId, 6, 1);

      vkCmdEndRendering(*cmdBuffer);
    });
}

void SSAORenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);

  vkDestroySampler(renderContext->device(), _sampler0, nullptr);
  vkDestroySampler(renderContext->device(), _depthSampler, nullptr);
  vkDestroySampler(renderContext->device(), _noiseSampler, nullptr);

  auto im = (ImageRenderResource*)vault->getResource("SSAOImage");
  auto imView = (ImageViewRenderResource*)vault->getResource("SSAOImageView");

  auto noiseIm = (ImageRenderResource*)vault->getResource("SSAONoiseImage");
  auto noiseImView = (ImageViewRenderResource*)vault->getResource("SSAONoiseImageView");

  vmaDestroyImage(renderContext->vmaAllocator(), noiseIm->_image._image, noiseIm->_image._allocation);
  vkDestroyImageView(renderContext->device(), noiseImView->_view, nullptr);

  vmaDestroyImage(renderContext->vmaAllocator(), im->_image._image, im->_image._allocation);
  vkDestroyImageView(renderContext->device(), imView->_view, nullptr);

  vmaDestroyBuffer(renderContext->vmaAllocator(), _sampleBuffer._buffer, _sampleBuffer._allocation);

  vault->deleteResource("SSAOImage");
  vault->deleteResource("SSAOImageView");
}

}