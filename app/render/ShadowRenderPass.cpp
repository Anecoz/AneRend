#include "ShadowRenderPass.h"

#include "ImageHelpers.h"

#include "FrameGraphBuilder.h"
#include "RenderContext.h"
#include "RenderResource.h"
#include "RenderResourceVault.h"

namespace render {

ShadowRenderPass::ShadowRenderPass()
  : RenderPass()
{
}

ShadowRenderPass::~ShadowRenderPass()
{
}

bool ShadowRenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Desc layout for translation buffer
  DescriptorBindInfo transBufferInfo{};
  transBufferInfo.binding = 0;
  transBufferInfo.stages = VK_SHADER_STAGE_VERTEX_BIT;
  transBufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(transBufferInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptor (multi buffered) containing draw buffer SSBO
  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  auto allocator = renderContext->vmaAllocator();
  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    auto buf = (BufferRenderResource*)vault->getResource("CullTransBuf", i);

    transBufferInfo.buffer = buf->_buffer._buffer;
    descParam.bindInfos.emplace_back(transBufferInfo);
  }

  _descriptorSets = buildDescriptorSets(descParam);

  // Build graphics pipeline
  GraphicsPipelineCreateParams params{};
  params.device = renderContext->device();
  params.pipelineLayout = _pipelineLayout;
  params.vertShader = "standard_shadow_vert.spv";
  params.colorLoc = -1;
  params.normalLoc = -1;
  params.colorAttachment = false;
  params.depthBiasEnable = true;
  params.depthBiasConstant = 4.0f;
  params.depthBiasSlope = 1.5f;

  buildGraphicsPipeline(params);

  // Create the shadow map
  auto shadowMapRes = new ImageRenderResource();
  auto shadowMapViewRes = new ImageViewRenderResource();

  shadowMapRes->_format = params.depthFormat;
  shadowMapViewRes->_format = shadowMapRes->_format;

  imageutil::createImage(
    1024,
    1024,
    params.depthFormat,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    shadowMapRes->_image);

  shadowMapViewRes->_view = imageutil::createImageView(
    renderContext->device(),
    shadowMapRes->_image._image,
    shadowMapRes->_format,
    VK_IMAGE_ASPECT_DEPTH_BIT);

  vault->addResource("ShadowMap", std::unique_ptr<IRenderResource>(shadowMapRes));
  vault->addResource("ShadowMapView", std::unique_ptr<IRenderResource>(shadowMapViewRes));

  return true;
}

void ShadowRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "Shadow";

  {
    ResourceUsage shadowMapUsage{};
    shadowMapUsage._resourceName = "ShadowMap";
    shadowMapUsage._access.set((std::size_t)Access::Write);
    shadowMapUsage._stage.set((std::size_t)Stage::Fragment);
    shadowMapUsage._type = Type::DepthAttachment;
    info._resourceUsages.emplace_back(std::move(shadowMapUsage));
  }

  {
    ResourceUsage usage{};
    usage._resourceName = "CullTransBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Vertex);
    usage._type = Type::SSBO;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "CullDrawBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::IndirectDraw);
    usage._type = Type::SSBO;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("Shadow",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra)
    {
      auto shadowMapView = (ImageViewRenderResource*)vault->getResource("ShadowMapView");
      auto drawCallBuffer = (BufferRenderResource*)vault->getResource("CullDrawBuf");

      VkExtent2D extent{};
      extent.width = 1024;
      extent.height = 1024;

      VkClearValue clearValue;
      clearValue.depthStencil = { 1.0f, 0 };

      VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
      depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      depthAttachmentInfo.imageView = shadowMapView->_view;
      depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
      depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depthAttachmentInfo.clearValue = clearValue;

      VkRenderingInfoKHR renderInfo{};
      renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderInfo.renderArea.extent = extent;
      renderInfo.renderArea.offset = { 0, 0 };
      renderInfo.layerCount = 1;
      renderInfo.colorAttachmentCount = 0;
      renderInfo.pDepthAttachment = &depthAttachmentInfo;

      vkCmdBeginRendering(*cmdBuffer, &renderInfo);

      // Viewport and scissor
      VkViewport viewport{};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(extent.width);
      viewport.height = static_cast<float>(extent.height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor{};
      scissor.offset = { 0, 0 };
      scissor.extent = extent;

      // Bind pipeline
      vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

      // Descriptor set for translation buffer in set 1
      vkCmdBindDescriptorSets(
        *cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout,
        1, 1, &_descriptorSets[multiBufferIdx],
        0, nullptr);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*cmdBuffer, 0, 1, &scissor);

      // Ask render context to draw big giga buffer
      renderContext->drawGigaBufferIndirect(cmdBuffer, drawCallBuffer->_buffer._buffer);

      // Stop dynamic rendering
      vkCmdEndRendering(*cmdBuffer);
    });
}

void ShadowRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  auto depthView = (ImageViewRenderResource*)vault->getResource("ShadowMapView");
  auto depthIm = (ImageRenderResource*)vault->getResource("ShadowMap");

  vmaDestroyImage(renderContext->vmaAllocator(), depthIm->_image._image, depthIm->_image._allocation);
  vkDestroyImageView(renderContext->device(), depthView->_view, nullptr);

  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);

  vault->deleteResource("ShadowMapView");
  vault->deleteResource("ShadowMap");
}

}