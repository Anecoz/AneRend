#include "SSAOBlurRenderPass.h"

#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../FrameGraphBuilder.h"
#include "../ImageHelpers.h"
#include "../../util/GraphicsUtils.h"

namespace render {

SSAOBlurRenderPass::SSAOBlurRenderPass()
  : RenderPass()
{}

SSAOBlurRenderPass::~SSAOBlurRenderPass()
{}

void SSAOBlurRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Screen quad
  _screenQuad._vertices = graphicsutil::createScreenQuad(1.0f, 1.0f);
  _meshId = rc->registerMesh(_screenQuad);

  RenderPassRegisterInfo info{};
  info._name = "SSAOBlur";

  std::vector<ResourceUsage> resourceUsages;
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOBlurImage";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialHeight = rc->swapChainExtent().height;
    createInfo._initialWidth = rc->swapChainExtent().width;
    createInfo._intialFormat = VK_FORMAT_R32_SFLOAT;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ColorAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsages);

  GraphicsPipelineCreateParams param{};
  param.device = rc->device();
  param.vertShader = "fullscreen_vert.spv";
  param.fragShader = "ssao_blur_frag.spv";
  param.depthTest = false;
  param.normalLoc = -1;
  param.colorLoc = -1;
  param.uvLoc = 1;
  param.colorFormats = { VK_FORMAT_R32_SFLOAT };
  info._graphicsParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("SSAOBlur",
    [this](RenderExeParams exeParams) {
      VkClearValue clearValue{};
      clearValue.color = { {1.0f, 1.0f, 1.0f, 1.0f} };
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = exeParams.colorAttachmentViews[0];
      colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachmentInfo.clearValue = clearValue;

      VkRenderingInfoKHR renderInfo{};
      renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderInfo.renderArea.extent = exeParams.rc->swapChainExtent();
      renderInfo.renderArea.offset = { 0, 0 };
      renderInfo.layerCount = 1;
      renderInfo.colorAttachmentCount = 1;
      renderInfo.pColorAttachments = &colorAttachmentInfo;
      renderInfo.pDepthAttachment = nullptr;

      vkCmdBeginRendering(*exeParams.cmdBuffer, &renderInfo);

      // Viewport and scissor
      VkViewport viewport{};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(exeParams.rc->swapChainExtent().width);
      viewport.height = static_cast<float>(exeParams.rc->swapChainExtent().height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor{};
      scissor.offset = { 0, 0 };
      scissor.extent = exeParams.rc->swapChainExtent();

      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*exeParams.cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*exeParams.cmdBuffer, 0, 1, &scissor);

      exeParams.rc->drawMeshId(exeParams.cmdBuffer, _meshId, 6, 1);

      vkCmdEndRendering(*exeParams.cmdBuffer);
    });
}

}