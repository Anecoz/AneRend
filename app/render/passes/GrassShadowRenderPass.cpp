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

void GrassShadowRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "GrassShadow";
  info._group = "Shadow";

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
    usage._multiBuffered = true;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "CullGrassDrawBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::IndirectDraw);
    usage._type = Type::SSBO;
    usage._multiBuffered = true;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  GraphicsPipelineCreateParams params{};
  params.device = rc->device();
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
  info._graphicsParams = params;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("GrassShadow",
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

      // Descriptor set for grass obj buffer
      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[exeParams.rc->getCurrentMultiBufferIdx()],
        0, nullptr);

      // Push constant 1 for shadow
      uint32_t pushConstants = 1;
      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        0,
        sizeof(uint32_t),
        &pushConstants);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*exeParams.cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*exeParams.cmdBuffer, 0, 1, &scissor);

      exeParams.rc->drawNonIndexIndirect(exeParams.cmdBuffer, exeParams.buffers[1], 1, 0);

      // Stop dynamic rendering
      vkCmdEndRendering(*exeParams.cmdBuffer);
    });
}

}