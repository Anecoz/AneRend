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

bool SSAOBlurRenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Create the output render target
  auto extent = renderContext->swapChainExtent();

  auto outputRes = new ImageRenderResource();
  outputRes->_format = VK_FORMAT_R32_SFLOAT;

  auto outputViewRes = new ImageViewRenderResource();
  outputViewRes->_format = VK_FORMAT_R32_SFLOAT;

  imageutil::createImage(
    extent.width,
    extent.height,
    outputRes->_format,
    VK_IMAGE_TILING_OPTIMAL,
    renderContext->vmaAllocator(),
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    outputRes->_image);

  outputViewRes->_view = imageutil::createImageView(renderContext->device(), outputRes->_image._image, outputRes->_format, VK_IMAGE_ASPECT_COLOR_BIT);

  vault->addResource("SSAOBlurImage", std::unique_ptr<IRenderResource>(outputRes));
  vault->addResource("SSAOBlurImageView", std::unique_ptr<IRenderResource>(outputViewRes));

  // Desc layout for ssao sampler
  DescriptorBindInfo samplerInfo{};
  samplerInfo.binding = 0;
  samplerInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(samplerInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptor containing samplers
  auto im = (ImageViewRenderResource*)vault->getResource("SSAOImageView");

  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  SamplerCreateParams samplerParam{};
  samplerParam.renderContext = renderContext;
  _sampler = createSampler(samplerParam);

  samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  samplerInfo.sampler = _sampler;
  samplerInfo.view = im->_view;
  descParam.bindInfos.emplace_back(samplerInfo);

  descParam.multiBuffered = false;

  _descriptorSets = buildDescriptorSets(descParam);

  // Build a graphics pipeline
  GraphicsPipelineCreateParams param{};
  param.pipelineLayout = _pipelineLayout;
  param.device = renderContext->device();
  param.vertShader = "shadow_debug_vert.spv";
  param.fragShader = "ssao_blur_frag.spv";
  param.depthTest = false;
  param.normalLoc = -1;
  param.colorLoc = -1;
  param.uvLoc = 1;
  param.colorFormat = VK_FORMAT_R32_SFLOAT;

  buildGraphicsPipeline(param);

  // Screen quad
  _screenQuad._vertices = graphicsutil::createScreenQuad(1.0f, 1.0f);

  _meshId = renderContext->registerMesh(_screenQuad._vertices, {});
  _renderableId = renderContext->registerRenderable(_meshId, glm::mat4(1.0f), glm::vec3(1.0f), 0.0f);
  renderContext->setRenderableVisible(_renderableId, false);

  return true;
}

void SSAOBlurRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
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
    usage._type = Type::ColorAttachment;
    resourceUsages.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsages);
  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("SSAOBlur",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra) {
      // Get resources
      auto outputImView = (ImageViewRenderResource*)vault->getResource("SSAOBlurImageView");

      VkClearValue clearValue{};
      clearValue.color = { {1.0f, 1.0f, 1.0f, 1.0f} };
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = outputImView->_view;
      colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
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

      vkCmdBindDescriptorSets(
        *cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout,
        1, 1, &_descriptorSets[0],
        0, nullptr);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*cmdBuffer, 0, 1, &scissor);

      renderContext->drawMeshId(cmdBuffer, _meshId, 6, 1);

      vkCmdEndRendering(*cmdBuffer);
    });
}

void SSAOBlurRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);

  vkDestroySampler(renderContext->device(), _sampler, nullptr);

  auto im = (ImageRenderResource*)vault->getResource("SSAOBlurImage");
  auto imView = (ImageViewRenderResource*)vault->getResource("SSAOBlurImageView");

  vmaDestroyImage(renderContext->vmaAllocator(), im->_image._image, im->_image._allocation);
  vkDestroyImageView(renderContext->device(), imView->_view, nullptr);

  vault->deleteResource("SSAOBlurImage");
  vault->deleteResource("SSAOBlurImageView");
}

}