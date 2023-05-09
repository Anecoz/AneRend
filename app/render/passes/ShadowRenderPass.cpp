#include "ShadowRenderPass.h"

#include "../ImageHelpers.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../RenderResource.h"
#include "../RenderResourceVault.h"

namespace render {

ShadowRenderPass::ShadowRenderPass()
  : RenderPass()
{
}

ShadowRenderPass::~ShadowRenderPass()
{
}

void ShadowRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "Shadow";

  {
    ResourceUsage usage{};
    usage._resourceName = "CullTransBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Vertex);
    usage._type = Type::SSBO;
    usage._multiBuffered = true;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ShadowMap";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::DepthAttachment;
    ImageInitialCreateInfo createInfo{};
    createInfo._intialFormat = VK_FORMAT_D32_SFLOAT;
    createInfo._initialHeight = 2048;
    createInfo._initialWidth = 2048;
    usage._imageCreateInfo = createInfo;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "CullDrawBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::IndirectDraw);
    usage._type = Type::SSBO;
    usage._multiBuffered = true;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  GraphicsPipelineCreateParams params{};
  params.device = rc->device();
  params.vertShader = "standard_shadow_vert.spv";
  params.colorLoc = -1;
  params.normalLoc = -1;
  params.colorAttachment = false;
  params.depthBiasEnable = true;
  params.depthBiasConstant = 4.0f;
  params.depthBiasSlope = 1.5f;
  //params.cullMode = VK_CULL_MODE_FRONT_BIT;
  info._graphicsParams = params;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("Shadow",
    [this](RenderExeParams exeParams)
    {
      if (!exeParams.rc->getRenderOptions().directionalShadows) return;

      VkExtent2D extent{};
      extent.width = 2048;
      extent.height = 2048;

      VkClearValue clearValue;
      clearValue.depthStencil = { 1.0f, 0 };

      VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
      depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      depthAttachmentInfo.imageView = exeParams.depthAttachmentViews[0];
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

      vkCmdBeginRendering(*exeParams.cmdBuffer, &renderInfo);

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
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *exeParams.pipeline);

      // Descriptor set for translation buffer in set 1
      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[exeParams.rc->getCurrentMultiBufferIdx()],
        0, nullptr);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*exeParams.cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*exeParams.cmdBuffer, 0, 1, &scissor);

      // Ask render context to draw big giga buffer
      exeParams.rc->drawGigaBufferIndirect(exeParams.cmdBuffer, exeParams.buffers[1]);

      // Stop dynamic rendering
      vkCmdEndRendering(*exeParams.cmdBuffer);
    });
}

}