#include "GrassShadowRenderPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderResource.h"
#include "../RenderResourceVault.h"

namespace render {

GrassShadowRenderPass::GrassShadowRenderPass()
  : RenderPass()
{}

GrassShadowRenderPass::~GrassShadowRenderPass()
{}

bool GrassShadowRenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Desc layout for grass obj buffer
  DescriptorBindInfo bufferInfo{};
  bufferInfo.binding = 0;
  bufferInfo.stages = VK_SHADER_STAGE_VERTEX_BIT;
  bufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(bufferInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptor (multi buffered) containing grass obj SSBO
  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  auto allocator = renderContext->vmaAllocator();
  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    auto buf = (BufferRenderResource*)vault->getResource("CullGrassObjBuf", i);

    bufferInfo.buffer = buf->_buffer._buffer;
    descParam.bindInfos.emplace_back(bufferInfo);
  }

  _descriptorSets = buildDescriptorSets(descParam);

  // Build graphics pipeline
  GraphicsPipelineCreateParams params{};
  params.device = renderContext->device();
  params.pipelineLayout = _pipelineLayout;
  params.vertShader = "grass_vert.spv";
  params.colorLoc = -1;
  params.normalLoc = -1;
  params.colorAttachment = false;
  params.depthBiasEnable = true;
  params.depthBiasConstant = 4.0f;
  params.depthBiasSlope = 1.5f;
  params.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  params.vertexLess = true;
  params.cullMode = VK_CULL_MODE_NONE;

  buildGraphicsPipeline(params);

  return true;
}

void GrassShadowRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "GrassShadow";

  {
    ResourceUsage shadowMapUsage{};
    shadowMapUsage._resourceName = "ShadowMap";
    shadowMapUsage._access.set((std::size_t)Access::Write);
    shadowMapUsage._access.set((std::size_t)Access::Read);
    shadowMapUsage._stage.set((std::size_t)Stage::Fragment);
    shadowMapUsage._type = Type::DepthAttachment;
    info._resourceUsages.emplace_back(std::move(shadowMapUsage));
  }

  {
    ResourceUsage usage{};
    usage._resourceName = "CullGrassObjBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Vertex);
    usage._type = Type::SSBO;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "CullGrassDrawBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::IndirectDraw);
    usage._type = Type::SSBO;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("GrassShadow",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra)
    {
      auto shadowMapView = (ImageViewRenderResource*)vault->getResource("ShadowMapView");
      auto drawCallBuffer = (BufferRenderResource*)vault->getResource("CullGrassDrawBuf", multiBufferIdx);

      VkExtent2D extent{};
      extent.width = 4096;
      extent.height = 4096;

      VkClearValue clearValue;
      clearValue.depthStencil = { 1.0f, 0 };

      VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
      depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      depthAttachmentInfo.imageView = shadowMapView->_view;
      depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
      depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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

      // Descriptor set for grass obj buffer
      vkCmdBindDescriptorSets(
        *cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout,
        1, 1, &_descriptorSets[multiBufferIdx],
        0, nullptr);

      // Push constant 1 for shadow
      uint32_t pushConstants = 1;
      vkCmdPushConstants(
        *cmdBuffer,
        _pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(uint32_t),
        &pushConstants);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*cmdBuffer, 0, 1, &scissor);

      renderContext->drawNonIndexIndirect(cmdBuffer, drawCallBuffer->_buffer._buffer, 1, 0);

      // Stop dynamic rendering
      vkCmdEndRendering(*cmdBuffer);
    });
}

void GrassShadowRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);
}

}