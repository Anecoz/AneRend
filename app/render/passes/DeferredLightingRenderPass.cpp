#include "DeferredLightingRenderPass.h"

#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../FrameGraphBuilder.h"
#include "../../util/GraphicsUtils.h"
#include "../ImageHelpers.h"

namespace render {

DeferredLightingRenderPass::DeferredLightingRenderPass()
  : RenderPass()
{
}

DeferredLightingRenderPass::~DeferredLightingRenderPass()
{
}

bool DeferredLightingRenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Desc layout for gbuffer samplers
  DescriptorBindInfo sampler0Info{};
  sampler0Info.binding = 0;
  sampler0Info.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  sampler0Info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorBindInfo sampler1Info{};
  sampler1Info.binding = 1;
  sampler1Info.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  sampler1Info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorBindInfo depthSamplerInfo{};
  depthSamplerInfo.binding = 2;
  depthSamplerInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  depthSamplerInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorBindInfo shadowMapSamplerInfo{};
  shadowMapSamplerInfo.binding = 3;
  shadowMapSamplerInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  shadowMapSamplerInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(sampler0Info);
  descLayoutParam.bindInfos.emplace_back(sampler1Info);
  descLayoutParam.bindInfos.emplace_back(depthSamplerInfo);
  descLayoutParam.bindInfos.emplace_back(shadowMapSamplerInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptor containing samplers
  auto im0 = (ImageViewRenderResource*)vault->getResource("Geometry0ImageView");
  auto im1 = (ImageViewRenderResource*)vault->getResource("Geometry1ImageView");
  auto depth = (ImageViewRenderResource*)vault->getResource("GeometryDepthImageView");
  auto shadowMap = (ImageViewRenderResource*)vault->getResource("ShadowMapView");

  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  SamplerCreateParams samplerParam{};
  samplerParam.renderContext = renderContext;
  _sampler0 = createSampler(samplerParam);
  _sampler1 = createSampler(samplerParam);
  _sampler2 = createSampler(samplerParam);
  _depthSampler = createSampler(samplerParam);
  _shadowMapSampler = createSampler(samplerParam);

  sampler0Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  sampler0Info.sampler = _sampler0;
  sampler0Info.view = im0->_view;
  descParam.bindInfos.emplace_back(sampler0Info);

  sampler1Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  sampler1Info.sampler = _sampler1;
  sampler1Info.view = im1->_view;
  descParam.bindInfos.emplace_back(sampler1Info);

  depthSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  depthSamplerInfo.sampler = _depthSampler;
  depthSamplerInfo.view = depth->_view;
  descParam.bindInfos.emplace_back(depthSamplerInfo);

  shadowMapSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  shadowMapSamplerInfo.sampler = _shadowMapSampler;
  shadowMapSamplerInfo.view = shadowMap->_view;
  descParam.bindInfos.emplace_back(shadowMapSamplerInfo);

  descParam.multiBuffered = false;

  _descriptorSets = buildDescriptorSets(descParam);

  // Build a graphics pipeline
  GraphicsPipelineCreateParams param{};
  param.pipelineLayout = _pipelineLayout;
  param.device = renderContext->device();
  param.vertShader = "shadow_debug_vert.spv";
  param.fragShader = "deferred_pbr_frag.spv";
  param.depthTest = false;
  param.normalLoc = -1;
  param.colorLoc = -1;
  param.uvLoc = 1;

  buildGraphicsPipeline(param);

  // Screen quad
  _screenQuad._vertices = graphicsutil::createScreenQuad(1.0f, 1.0f);

  _meshId = renderContext->registerMesh(_screenQuad._vertices, {});
  _renderableId = renderContext->registerRenderable(_meshId, glm::mat4(1.0f), glm::vec3(1.0f), 0.0f);
  renderContext->setRenderableVisible(_renderableId, false);

  // Create the output image
  auto extent = renderContext->swapChainExtent();

  auto outputRes = new ImageRenderResource();
  outputRes->_format = VK_FORMAT_B8G8R8A8_SRGB;

  auto outputViewRes = new ImageViewRenderResource();
  outputViewRes->_format = VK_FORMAT_B8G8R8A8_SRGB;

  imageutil::createImage(
    extent.width,
    extent.height,
    outputRes->_format,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    outputRes->_image);

  outputViewRes->_view = imageutil::createImageView(renderContext->device(), outputRes->_image._image, outputRes->_format, VK_IMAGE_ASPECT_COLOR_BIT);

  vault->addResource("FinalImage", std::unique_ptr<IRenderResource>(outputRes));
  vault->addResource("FinalImageView", std::unique_ptr<IRenderResource>(outputViewRes));

  return true;
}

void DeferredLightingRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "DeferredPbrLight";

  std::vector<ResourceUsage> resourceUsages;
  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImage";
    usage._access.set((std::size_t)Access::Read);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::ColorAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }
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
    usage._resourceName = "Geometry1Image";
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
    usage._resourceName = "ShadowMap";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsages);
  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("DeferredPbrLight",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra) {
      // Get resources
      auto finalImView = (ImageViewRenderResource*)vault->getResource("FinalImageView");

      VkClearValue clearValue{};
      clearValue.color = { {0.5f, 0.5f, 0.2f, 1.0f} };
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = finalImView->_view;
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

void DeferredLightingRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);

  vkDestroySampler(renderContext->device(), _sampler0, nullptr);
  vkDestroySampler(renderContext->device(), _sampler1, nullptr);
  vkDestroySampler(renderContext->device(), _sampler2, nullptr);
  vkDestroySampler(renderContext->device(), _depthSampler, nullptr);
  vkDestroySampler(renderContext->device(), _shadowMapSampler, nullptr);

  auto im = (ImageRenderResource*)vault->getResource("FinalImage");
  auto imView = (ImageViewRenderResource*)vault->getResource("FinalImageView");

  vmaDestroyImage(renderContext->vmaAllocator(), im->_image._image, im->_image._allocation);
  vkDestroyImageView(renderContext->device(), imView->_view, nullptr);

  vault->deleteResource("FinalImage");
  vault->deleteResource("FinalImageView");
}

}