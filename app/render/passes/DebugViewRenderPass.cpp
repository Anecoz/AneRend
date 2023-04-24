#include "DebugViewRenderPass.h"

#include "../../util/GraphicsUtils.h"

#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../ImageHelpers.h"

namespace render {

DebugViewRenderPass::DebugViewRenderPass()
  : RenderPass()
{}

DebugViewRenderPass::~DebugViewRenderPass()
{}

uint32_t DebugViewRenderPass::translateNameToBinding(const std::string& name)
{
  for (int i = 0; i < _resourceUsages.size(); ++i) {
    if (_resourceUsages[i]._resourceName == name) {
      return i;
    }
  }

  return 0;
}

void DebugViewRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Screen quad
  _screenQuad._vertices = graphicsutil::createScreenQuad(0.25f, 0.25f);
  _meshId = rc->registerMesh(_screenQuad._vertices, {});

  _resourceUsages.clear();

  RenderPassRegisterInfo info{};
  info._name = "DebugView";

  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImagePP";
    usage._access.set((std::size_t)Access::Read);
    usage._access.set((std::size_t)Access::Write);
    usage._type = Type::ColorAttachment;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledDepthTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ShadowMap";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledDepthTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry0Image";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "Geometry1Image";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "WindForceImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSAOBlurImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }

  for (auto& us : _resourceUsages) {
    info._resourceUsages.emplace_back(us);
  }

  GraphicsPipelineCreateParams param{};
  param.device = rc->device();
  param.vertShader = "fullscreen_vert.spv";
  param.fragShader = "debug_view_frag.spv";
  param.depthTest = false;
  param.normalLoc = -1;
  param.colorLoc = -1;
  param.uvLoc = 1;
  param.colorFormats = { VK_FORMAT_R16G16B16A16_UNORM };
  info._graphicsParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("DebugView",
    [this](RenderExeParams exeParams)
    {
      if (!exeParams.rc->getDebugOptions().debugView) return;

      // If view is updated, descriptor (sampler) has to be recreated
      std::string debugResource = exeParams.rc->getDebugOptions().debugViewResource;
      uint32_t push = translateNameToBinding(debugResource);

      VkClearValue clearValue{};
      clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = exeParams.colorAttachmentViews[0];
      colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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

      // Descriptor set for translation buffer in set 1
      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*exeParams.cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*exeParams.cmdBuffer, 0, 1, &scissor);

      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(uint32_t),
        &push);

      exeParams.rc->drawMeshId(exeParams.cmdBuffer, _meshId, 6, 1);

      vkCmdEndRendering(*exeParams.cmdBuffer);
    });
}

}