#include "GrassRenderPass.h"

#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../FrameGraphBuilder.h"

namespace render {

GrassRenderPass::GrassRenderPass()
  : RenderPass()
{
}

GrassRenderPass::~GrassRenderPass()
{
}

bool GrassRenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Desc layout for translation buffer
  DescriptorBindInfo grassBufferInfo{};
  grassBufferInfo.binding = 0;
  grassBufferInfo.stages = VK_SHADER_STAGE_VERTEX_BIT;
  grassBufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(grassBufferInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptor (multi buffered) containing SSBO
  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    auto buf = (BufferRenderResource*)vault->getResource("CullGrassObjBuf", i);
    grassBufferInfo.buffer = buf->_buffer._buffer;

    descParam.bindInfos.emplace_back(grassBufferInfo);
  }

  _descriptorSets = buildDescriptorSets(descParam);

  // Build a graphics pipeline
  GraphicsPipelineCreateParams param{};
  param.pipelineLayout = _pipelineLayout;
  param.cullMode = VK_CULL_MODE_NONE;
  param.device = renderContext->device();
  param.vertShader = "grass_vert.spv";
  param.fragShader = "grass_frag.spv";
  param.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  param.vertexLess = true;

  buildGraphicsPipeline(param);

  return true;
}

void GrassRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "Grass";

  std::vector<ResourceUsage> resourceUsages;
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryColorImage";
    usage._access.set((std::size_t)Access::Write);
    usage._access.set((std::size_t)Access::Read);
    usage._type = Type::ColorAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Write);
    usage._access.set((std::size_t)Access::Read);
    usage._type = Type::DepthAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "CullGrassObjBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Vertex);
    usage._type = Type::SSBO;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "CullGrassDrawBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::IndirectDraw);
    usage._type = Type::SSBO;
    resourceUsages.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsages);
  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("Grass",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra)
    {
      // Get resources
      auto colorView = (ImageViewRenderResource*)vault->getResource("GeometryColorImageView");
      auto depthView = (ImageViewRenderResource*)vault->getResource("GeometryDepthImageView");
      auto drawCallBuffer = (BufferRenderResource*)vault->getResource("CullGrassDrawBuf", multiBufferIdx);

      // Dynamic rendering begin
      std::array<VkClearValue, 2> clearValues{};
      clearValues[0].color = { {0.5f, 0.5f, 0.5f, 1.0f} };
      clearValues[1].depthStencil = { 1.0f, 0 };
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = colorView->_view;
      colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachmentInfo.clearValue = clearValues[0];

      VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
      depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      depthAttachmentInfo.imageView = depthView->_view;
      depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
      depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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

      renderContext->drawNonIndexIndirect(cmdBuffer, drawCallBuffer->_buffer._buffer, 1, 0);

      // Stop dynamic rendering
      vkCmdEndRendering(*cmdBuffer);
    });
}

void GrassRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);
}

}