#include "GeometryRenderPass.h"

#include "RenderResource.h"
#include "ImageHelpers.h"
#include "FrameGraphBuilder.h"
#include "RenderResourceVault.h"

#include <array>
#include <memory>

namespace render {

GeometryRenderPass::GeometryRenderPass()
  : RenderPass()
{
}

GeometryRenderPass::~GeometryRenderPass()
{
}

void GeometryRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  auto colorView = (ImageViewRenderResource*)vault->getResource("GeometryColorImageView");
  auto colorIm = (ImageRenderResource*)vault->getResource("GeometryColorImage");
  auto depthView = (ImageViewRenderResource*)vault->getResource("GeometryDepthImageView");
  auto depthIm = (ImageRenderResource*)vault->getResource("GeometryDepthImage");

  vmaDestroyImage(renderContext->vmaAllocator(), colorIm->_image._image, colorIm->_image._allocation);
  vmaDestroyImage(renderContext->vmaAllocator(), depthIm->_image._image, depthIm->_image._allocation);
  vkDestroyImageView(renderContext->device(), colorView->_view, nullptr);
  vkDestroyImageView(renderContext->device(), depthView->_view, nullptr);

  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);

  vkDestroySampler(renderContext->device(), _shadowSampler, nullptr);

  vault->deleteResource("GeometryColorImageView");
  vault->deleteResource("GeometryColorImage");
  vault->deleteResource("GeometryDepthImageView");
  vault->deleteResource("GeometryDepthImage");
}

bool GeometryRenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Desc layout for translation buffer
  DescriptorBindInfo transBufferInfo{};
  transBufferInfo.binding = 0;
  transBufferInfo.stages = VK_SHADER_STAGE_VERTEX_BIT;
  transBufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  DescriptorBindInfo shadowSamplerInfo{};
  shadowSamplerInfo.binding = 1;
  shadowSamplerInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  shadowSamplerInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(transBufferInfo);
  descLayoutParam.bindInfos.emplace_back(shadowSamplerInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptor (multi buffered) containing draw buffer SSBO
  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  // Sampler for shadow map
  SamplerCreateParams samplerParam{};
  samplerParam.renderContext = renderContext;
  _shadowSampler = createSampler(samplerParam);

  auto shadowMapView = (ImageViewRenderResource*)vault->getResource("ShadowMapView");

  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    auto buf = (BufferRenderResource*)vault->getResource("CullTransBuf", i);

    transBufferInfo.buffer = buf->_buffer._buffer;
    shadowSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    shadowSamplerInfo.sampler = _shadowSampler;
    shadowSamplerInfo.view = shadowMapView->_view;

    descParam.bindInfos.emplace_back(transBufferInfo);
    descParam.bindInfos.emplace_back(shadowSamplerInfo);
  }

  _descriptorSets = buildDescriptorSets(descParam);

  // Build a graphics pipeline
  GraphicsPipelineCreateParams param{};
  param.pipelineLayout = _pipelineLayout;
  param.device = renderContext->device();
  param.vertShader = "standard_vert.spv";
  param.fragShader = "standard_frag.spv";

  buildGraphicsPipeline(param);

  // Create the depth and color images used for rendering.
  auto extent = renderContext->swapChainExtent();

  // Depth
  auto depthRes = new ImageRenderResource();
  depthRes->_format = VK_FORMAT_D32_SFLOAT;

  auto depthViewRes = new ImageViewRenderResource();
  depthViewRes->_format = VK_FORMAT_D32_SFLOAT;

  imageutil::createImage(
    extent.width,
    extent.height,
    depthRes->_format,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    depthRes->_image);

  depthViewRes->_view = imageutil::createImageView(renderContext->device(), depthRes->_image._image, depthRes->_format, VK_IMAGE_ASPECT_DEPTH_BIT);

  vault->addResource("GeometryDepthImage", std::unique_ptr<IRenderResource>(depthRes));
  vault->addResource("GeometryDepthImageView", std::unique_ptr<IRenderResource>(depthViewRes));

  // Color
  auto colRes = new ImageRenderResource();
  auto colViewRes = new ImageViewRenderResource();

  colRes->_format = VK_FORMAT_B8G8R8A8_SRGB;
  colViewRes->_format = VK_FORMAT_B8G8R8A8_SRGB;

  imageutil::createImage(
    extent.width,
    extent.height,
    colRes->_format,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    colRes->_image);

  colViewRes->_view = imageutil::createImageView(renderContext->device(), colRes->_image._image, colRes->_format, VK_IMAGE_ASPECT_COLOR_BIT);

  vault->addResource("GeometryColorImage", std::unique_ptr<IRenderResource>(colRes));
  vault->addResource("GeometryColorImageView", std::unique_ptr<IRenderResource>(colViewRes));

  return true;
}

void GeometryRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "Geometry";

  std::vector<ResourceUsage> resourceUsages;
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryColorImage";
    usage._access.set((std::size_t)Access::Write);
    usage._type = Type::ColorAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Write);
    usage._type = Type::DepthAttachment;
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
  {
    ResourceUsage usage{};
    usage._resourceName = "CullTransBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Vertex);
    usage._type = Type::SSBO;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "CullDrawBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::IndirectDraw);
    usage._type = Type::SSBO;
    resourceUsages.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsages);
  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("Geometry",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra) {
      // Get resources
      auto colorView = (ImageViewRenderResource*)vault->getResource("GeometryColorImageView");
      auto depthView = (ImageViewRenderResource*)vault->getResource("GeometryDepthImageView");
      auto drawCallBuffer = (BufferRenderResource*)vault->getResource("CullDrawBuf");

      // Dynamic rendering begin
      std::array<VkClearValue, 2> clearValues{};
      clearValues[0].color = { {0.5f, 0.5f, 0.5f, 1.0f} };
      clearValues[1].depthStencil = { 1.0f, 0 };
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = colorView->_view;
      colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachmentInfo.clearValue = clearValues[0];

      VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
      depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      depthAttachmentInfo.imageView = depthView->_view;
      depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
      depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depthAttachmentInfo.clearValue = clearValues[1];

      VkRenderingInfoKHR renderInfo{};
      renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderInfo.renderArea.extent = renderContext->swapChainExtent();
      renderInfo.renderArea.offset = { 0, 0 };
      renderInfo.layerCount = 1;
      renderInfo.colorAttachmentCount = 1;
      renderInfo.pColorAttachments = &colorAttachmentInfo;
      renderInfo.pDepthAttachment = &depthAttachmentInfo;

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
    }
  );
}

}