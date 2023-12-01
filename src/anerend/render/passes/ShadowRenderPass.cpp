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
  reigsterDirectionalShadowPass(fgb, rc);
  registerPointShadowPass(fgb, rc);
}

void ShadowRenderPass::reigsterDirectionalShadowPass(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "Shadow";
  info._group = "Shadow";

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
  params.jointLoc = 1;
  params.jointWeightLoc = 2;
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
      exeParams.rc->drawGigaBufferIndirect(exeParams.cmdBuffer, exeParams.buffers[1], static_cast<uint32_t>(exeParams.rc->getCurrentMeshes().size()));

      // Stop dynamic rendering
      vkCmdEndRendering(*exeParams.cmdBuffer);
    });
}

void ShadowRenderPass::registerPointShadowPass(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "PointShadow";
  info._group = "PointShadow"; // TODO: Put in shadow group?

  {
    ResourceUsage usage{};
    usage._resourceName = "CullTransBuf";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Vertex);
    usage._type = Type::SSBO;
    usage._multiBuffered = true;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  for (std::size_t i = 0; i < rc->getMaxNumPointLightShadows(); ++i) {
    ResourceUsage usage{};
    usage._resourceName = "PointShadowMap" + std::to_string(i);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::DepthAttachment;

    ImageInitialCreateInfo createInfo{};
    createInfo._intialFormat = VK_FORMAT_D32_SFLOAT;
    createInfo._initialHeight = 512;
    createInfo._initialWidth = 512;
    createInfo._arrayLayers = 6;
    createInfo._cubeCompat = true;
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
  params.vertShader = "standard_shadow_point_vert.spv";
  params.fragShader = "standard_shadow_frag.spv";
  params.colorLoc = -1;
  params.normalLoc = -1;
  params.jointLoc = 1;
  params.jointWeightLoc = 2;
  params.colorAttachment = false;
  params.depthBiasEnable = true;
  params.depthBiasConstant = 4.0f;
  params.depthBiasSlope = 1.5f;
  params.viewMask = (uint32_t)0b111111; // This enables multiview, the bitmask enables all 6 views of the cube
  //params.cullMode = VK_CULL_MODE_FRONT_BIT;
  info._graphicsParams = params;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("PointShadow",
    [this](RenderExeParams exeParams)
    {
      if (!exeParams.rc->getRenderOptions().pointShadows) return;

      auto lightIndices = exeParams.rc->getShadowCasterLightIndices();

      for (std::size_t i = 0; i < lightIndices.size(); ++i) {

        if (lightIndices[i] == -1) {
          continue;
        }

        auto& light = exeParams.rc->getLights()[lightIndices[i]];

        // Use multiview rendering to just do 1 draw call per cube.
        // The first view in the attachment is the cube view.
        auto& cubeView = exeParams.depthAttachmentCubeViews[7 * i];

        VkExtent2D extent{};
        extent.width = 512;
        extent.height = 512;

        VkClearValue clearValue;
        clearValue.depthStencil = { 1.0f, 0 };

        VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
        depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthAttachmentInfo.imageView = cubeView;
        depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachmentInfo.clearValue = clearValue;

        VkRenderingInfoKHR renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderInfo.renderArea.extent = extent;
        renderInfo.renderArea.offset = { 0, 0 };
        renderInfo.viewMask = (uint32_t)0b111111; // This enables multiview, the bitmask enables all 6 views of the cube
        renderInfo.layerCount = 1; // Not sure about layerCount when multiview is enabled, the spec is a bit unclear
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

        struct Push
        {
          glm::vec4 params;
          unsigned index;
        } push;

        push.index = (unsigned)i;
        push.params = glm::vec4(light._pos, light._range);

        vkCmdPushConstants(
          *exeParams.cmdBuffer,
          *exeParams.pipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
          0,
          sizeof(Push),
          &push);

        // Ask render context to draw big giga buffer
        exeParams.rc->drawGigaBufferIndirect(exeParams.cmdBuffer, exeParams.buffers[1], static_cast<uint32_t>(exeParams.rc->getCurrentMeshes().size()));

        // Stop dynamic rendering
        vkCmdEndRendering(*exeParams.cmdBuffer);
      }
    });
}

}