#include "DebugViewRenderPass.h"

#include "../../util/GraphicsUtils.h"

#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../FrameGraphBuilder.h"
#include "../ImageHelpers.h"

namespace render {

DebugViewRenderPass::DebugViewRenderPass()
  : RenderPass()
{
}

DebugViewRenderPass::~DebugViewRenderPass()
{
}

bool DebugViewRenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Desc layout
  DescriptorBindInfo samplerInfo{};
  samplerInfo.binding = 0;
  samplerInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(samplerInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptor
  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  // Sampler for shadow map
  SamplerCreateParams samplerParam{};
  samplerParam.renderContext = renderContext;
  _sampler = createSampler(samplerParam);

  auto shadowMapView = (ImageViewRenderResource*)vault->getResource("ShadowMapView");

  samplerInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  samplerInfo.sampler = _sampler;
  samplerInfo.view = shadowMapView->_view;

  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    descParam.bindInfos.emplace_back(samplerInfo);
  }

  _descriptorSets = buildDescriptorSets(descParam);

  // Build a graphics pipeline
  GraphicsPipelineCreateParams param{};
  param.pipelineLayout = _pipelineLayout;
  param.device = renderContext->device();
  param.vertShader = "shadow_debug_vert.spv";
  param.fragShader = "shadow_debug_frag.spv";
  param.depthTest = false;
  param.normalLoc = -1;
  param.colorLoc = -1;
  param.uvLoc = 1;

  buildGraphicsPipeline(param);

  // Screen quad
  _screenQuad._vertices = graphicsutil::createScreenQuad(0.25f, 0.25f);

  _meshId = renderContext->registerMesh(_screenQuad._vertices, {});
  _renderableId = renderContext->registerRenderable(_meshId, glm::mat4(1.0f), glm::vec3(1.0f), 0.0f);
  renderContext->setRenderableVisible(_renderableId, false);

  _currResource = "ShadowMap";

  return true;
}

void DebugViewRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "DebugView";

  {
    ResourceUsage usage{};
    usage._resourceName = "ShadowMap";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryColorImage";
    usage._access.set((std::size_t)Access::Read);
    usage._access.set((std::size_t)Access::Write);
    usage._type = Type::ColorAttachment;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("DebugView",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra)
    {
      if (!renderContext->getDebugOptions().debugView) return;

      auto imageView = (ImageViewRenderResource*)vault->getResource("GeometryColorImageView");

      // If view is updated, descriptor (sampler) has to be recreated
      std::string debugResource = renderContext->getDebugOptions().debugViewResource;
      if (debugResource != _currResource) {
        std::string viewName = debugResource + "View";

        if (auto view = (ImageViewRenderResource*)vault->getResource(viewName)) {

          // Recreate descriptors
          DescriptorBindInfo samplerInfo{};
          samplerInfo.binding = 0;
          samplerInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
          samplerInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

          DescriptorSetsCreateParams descParam{};
          descParam.renderContext = renderContext;
          samplerInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
          samplerInfo.sampler = _sampler;
          samplerInfo.view = view->_view;

          for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
            descParam.bindInfos.emplace_back(samplerInfo);
          }

          updateDescriptorSets(descParam, _descriptorSets);
          _currResource = debugResource;
        }
      }

      VkClearValue clearValue{};
      clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = imageView->_view;
      colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachmentInfo.clearValue = clearValue;

      VkRenderingInfoKHR renderInfo{};
      renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderInfo.renderArea.extent = renderContext->swapChainExtent();
      renderInfo.renderArea.offset = { 0, 0 };
      renderInfo.layerCount = 1;
      renderInfo.colorAttachmentCount = 1;
      renderInfo.pColorAttachments = &colorAttachmentInfo;
      renderInfo.pDepthAttachment = nullptr;

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

      renderContext->drawMeshId(cmdBuffer, _meshId, 6, 1);

      vkCmdEndRendering(*cmdBuffer);
    });
}

void DebugViewRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);

  vkDestroySampler(renderContext->device(), _sampler, nullptr);
}

}