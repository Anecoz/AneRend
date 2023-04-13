#include "GeometryRenderPass.h"

#include "../RenderResource.h"
#include "../ImageHelpers.h"
#include "../FrameGraphBuilder.h"
#include "../RenderResourceVault.h"

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
  auto color0View = (ImageViewRenderResource*)vault->getResource("Geometry0ImageView");
  auto color1View = (ImageViewRenderResource*)vault->getResource("Geometry1ImageView");
  auto color2View = (ImageViewRenderResource*)vault->getResource("Geometry2ImageView");
  auto color0Im = (ImageRenderResource*)vault->getResource("Geometry0Image");
  auto color1Im = (ImageRenderResource*)vault->getResource("Geometry1Image");
  auto color2Im = (ImageRenderResource*)vault->getResource("Geometry2Image");
  auto depthView = (ImageViewRenderResource*)vault->getResource("GeometryDepthImageView");
  auto depthIm = (ImageRenderResource*)vault->getResource("GeometryDepthImage");

  vmaDestroyImage(renderContext->vmaAllocator(), color0Im->_image._image, color0Im->_image._allocation);
  vmaDestroyImage(renderContext->vmaAllocator(), color1Im->_image._image, color1Im->_image._allocation);
  vmaDestroyImage(renderContext->vmaAllocator(), color2Im->_image._image, color2Im->_image._allocation);
  vmaDestroyImage(renderContext->vmaAllocator(), depthIm->_image._image, depthIm->_image._allocation);
  vkDestroyImageView(renderContext->device(), color0View->_view, nullptr);
  vkDestroyImageView(renderContext->device(), color1View->_view, nullptr);
  vkDestroyImageView(renderContext->device(), color2View->_view, nullptr);
  vkDestroyImageView(renderContext->device(), depthView->_view, nullptr);

  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);

  vault->deleteResource("Geometry0ImageView");
  vault->deleteResource("Geometry0Image");
  vault->deleteResource("Geometry1ImageView");
  vault->deleteResource("Geometry1Image");
  vault->deleteResource("Geometry2ImageView");
  vault->deleteResource("Geometry2Image");
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

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(transBufferInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptor (multi buffered) containing draw buffer SSBO
  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    auto buf = (BufferRenderResource*)vault->getResource("CullTransBuf", i);

    transBufferInfo.buffer = buf->_buffer._buffer;
    descParam.bindInfos.emplace_back(transBufferInfo);
  }

  _descriptorSets = buildDescriptorSets(descParam);

  // Build a graphics pipeline
  VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
  GraphicsPipelineCreateParams param{};
  param.pipelineLayout = _pipelineLayout;
  param.device = renderContext->device();
  param.vertShader = "standard_vert.spv";
  param.fragShader = "standard_frag.spv";
  param.colorAttachmentCount = 3;
  param.colorFormats = { format, format, VK_FORMAT_R32G32B32A32_SFLOAT };

  buildGraphicsPipeline(param);

  // Create the gbuffer.
  // 1 depth attachment (32 bit float)
  // 3 rgba 4 channel 16 bit per channel color targets
  // - r0g0b0: normals
  // - a0r1g1: albedo
  // - b1: metallic
  // - a1: roughness
  // - r2g2b2: world pos
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
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    depthRes->_image);

  depthViewRes->_view = imageutil::createImageView(renderContext->device(), depthRes->_image._image, depthRes->_format, VK_IMAGE_ASPECT_DEPTH_BIT);

  vault->addResource("GeometryDepthImage", std::unique_ptr<IRenderResource>(depthRes));
  vault->addResource("GeometryDepthImageView", std::unique_ptr<IRenderResource>(depthViewRes));

  // Color images
  auto col0Res = new ImageRenderResource();
  auto col0ViewRes = new ImageViewRenderResource();
  auto col1Res = new ImageRenderResource();
  auto col1ViewRes = new ImageViewRenderResource();
  auto col2Res = new ImageRenderResource();
  auto col2ViewRes = new ImageViewRenderResource();

  // VK_FORMAT_B8G8R8A8_SRGB
  // VK_FORMAT_R16G16B16A16_SFLOAT
  col0Res->_format = format;
  col0ViewRes->_format = format;
  col1Res->_format = format;
  col1ViewRes->_format = format;
  col2Res->_format = VK_FORMAT_R32G32B32A32_SFLOAT;
  col2ViewRes->_format = VK_FORMAT_R32G32B32A32_SFLOAT;

  imageutil::createImage(
    extent.width,
    extent.height,
    col0Res->_format,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    col0Res->_image);

  col0ViewRes->_view = imageutil::createImageView(renderContext->device(), col0Res->_image._image, col0Res->_format, VK_IMAGE_ASPECT_COLOR_BIT);

  imageutil::createImage(
    extent.width,
    extent.height,
    col1Res->_format,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    col1Res->_image);

  col1ViewRes->_view = imageutil::createImageView(renderContext->device(), col1Res->_image._image, col1Res->_format, VK_IMAGE_ASPECT_COLOR_BIT);

  imageutil::createImage(
    extent.width,
    extent.height,
    col2Res->_format,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    col2Res->_image);

  col2ViewRes->_view = imageutil::createImageView(renderContext->device(), col2Res->_image._image, col2Res->_format, VK_IMAGE_ASPECT_COLOR_BIT);

  vault->addResource("Geometry0Image", std::unique_ptr<IRenderResource>(col0Res));
  vault->addResource("Geometry0ImageView", std::unique_ptr<IRenderResource>(col0ViewRes));
  vault->addResource("Geometry1Image", std::unique_ptr<IRenderResource>(col1Res));
  vault->addResource("Geometry1ImageView", std::unique_ptr<IRenderResource>(col1ViewRes));
  vault->addResource("Geometry2Image", std::unique_ptr<IRenderResource>(col2Res));
  vault->addResource("Geometry2ImageView", std::unique_ptr<IRenderResource>(col2ViewRes));

  return true;
}

void GeometryRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "Geometry";

  std::vector<ResourceUsage> resourceUsages;
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry0Image";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::ColorAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry1Image";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::ColorAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry2Image";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::ColorAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::DepthAttachment;
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
      auto color0View = (ImageViewRenderResource*)vault->getResource("Geometry0ImageView");
      auto color1View = (ImageViewRenderResource*)vault->getResource("Geometry1ImageView");
      auto color2View = (ImageViewRenderResource*)vault->getResource("Geometry2ImageView");
      auto depthView = (ImageViewRenderResource*)vault->getResource("GeometryDepthImageView");
      auto drawCallBuffer = (BufferRenderResource*)vault->getResource("CullDrawBuf", multiBufferIdx);

      // Dynamic rendering begin
      std::array<VkClearValue, 2> clearValues{};
      std::array<VkRenderingAttachmentInfoKHR, 3> colInfo{};
      clearValues[0].color = { {0.5f, 0.5f, 0.5f, 1.0f} };
      clearValues[1].depthStencil = { 1.0f, 0 };
      VkRenderingAttachmentInfoKHR color0AttachmentInfo{};
      color0AttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      color0AttachmentInfo.imageView = color0View->_view;
      color0AttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color0AttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      color0AttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      color0AttachmentInfo.clearValue = clearValues[0];

      VkRenderingAttachmentInfoKHR color1AttachmentInfo{};
      color1AttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      color1AttachmentInfo.imageView = color1View->_view;
      color1AttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color1AttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      color1AttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      color1AttachmentInfo.clearValue = clearValues[0];

      VkRenderingAttachmentInfoKHR color2AttachmentInfo{};
      color2AttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      color2AttachmentInfo.imageView = color2View->_view;
      color2AttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color2AttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      color2AttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      color2AttachmentInfo.clearValue = clearValues[0];

      colInfo[0] = color0AttachmentInfo;
      colInfo[1] = color1AttachmentInfo;
      colInfo[2] = color2AttachmentInfo;

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
      renderInfo.colorAttachmentCount = colInfo.size();
      renderInfo.pColorAttachments = colInfo.data();
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