#include "DebugViewRenderPass.h"

#include "../../util/GraphicsUtils.h"

#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../ImageHelpers.h"

namespace render {

struct Push {
  int32_t samplerId;
  float mip;
};

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
  asset::Model quadModel{};
  asset::Mesh quadMesh{};
  _meshId = quadMesh._id;
  quadMesh._vertices = util::createScreenQuad(0.75f, 0.75f);
  quadModel._meshes.emplace_back(std::move(quadMesh));

  AssetUpdate upd{};
  upd._addedModels.emplace_back(std::move(quadModel));
  rc->assetUpdate(std::move(upd));

  _resourceUsages.clear();

  RenderPassRegisterInfo info{};
  info._name = "DebugView";
  info._group = "Debug";

  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImagePP";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
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
    usage._resourceName = "Geometry2Image";
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
  {
    ResourceUsage usage{};
    usage._resourceName = "HiZ64";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "HiZ32";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "HiZ16";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "HiZ8";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "HiZ4";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "HiZ2";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "RTShadowImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysIrrTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysDirTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysConvTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ReflectTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    //usage._allMips = false;
    //usage._mip = 9;
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "TempProbeTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SpecularBRDFLutTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    usage._bindless = true;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "BloomDsTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    usage._samplerClampToEdge = true;
    //usage._noSamplerFiltering = true;
    usage._bindless = true;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "BloomUsTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    usage._samplerClampToEdge = true;
    //usage._noSamplerFiltering = true;
    usage._bindless = true;
    _resourceUsages.emplace_back(std::move(usage));
  }
  /*for (int i = 0; i < 1; ++i) {
    {
      ResourceUsage usage{};
      usage._resourceName = "SurfelDirTex" + std::to_string(i);
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Fragment);
      usage._type = Type::SampledTexture;
      usage._samplerClampToEdge = true;
      usage._bindless = true;
      _resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "SurfelIrrTex" + std::to_string(i);
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Fragment);
      usage._type = Type::SampledTexture;
      usage._samplerClampToEdge = true;
      usage._bindless = true;
      _resourceUsages.emplace_back(std::move(usage));
    }
     {
      ResourceUsage usage{};
      usage._resourceName = "SurfelSH" + std::to_string(i);
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Fragment);
      usage._type = Type::SampledTexture;
      usage._samplerClampToEdge = true;
      usage._bindless = true;
      _resourceUsages.emplace_back(std::move(usage));
    }
    for (int j = 0; j < 9; ++j) {
      ResourceUsage usage{};
      usage._resourceName = "SurfelSHLM" + std::to_string(i) + "_" + std::to_string(j);
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Fragment);
      usage._type = Type::SampledTexture;
      usage._samplerClampToEdge = true;
      usage._bindless = true;
      _resourceUsages.emplace_back(std::move(usage));
    }
  }*/


  /* {
    ResourceUsage usage{};
    usage._resourceName = "LightShadowDistTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    usage._samplerClampToEdge = true;
    usage._bindless = true;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "LightShadowDirTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    usage._samplerClampToEdge = true;
    usage._bindless = true;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "LightShadowMeanTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::SampledTexture;
    usage._samplerClampToBorder = true;
    usage._bindless = true;
    _resourceUsages.emplace_back(std::move(usage));
  }*/
  /* {
    ResourceUsage usage{};
    usage._resourceName = "SurfelTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SurfelConvTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }*/
  /*for (int x = 0; x < 60; ++x)
  for (int y = 0; y < 34; ++y) {
    ResourceUsage usage{};
    usage._resourceName = "SurfelTex_" + std::to_string(x) + "_" + std::to_string(y);
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }*/
  /* {
    ResourceUsage usage{};
    usage._resourceName = "SSGITex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "SSGIBlurTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._bindless = true;
    usage._type = Type::SampledTexture;
    _resourceUsages.emplace_back(std::move(usage));
  }*/

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
  param.colorFormats = { rc->getHDRFormat()};
  info._graphicsParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("DebugView",
    [this](RenderExeParams exeParams)
    {
      if (!exeParams.rc->getDebugOptions().debugView) return;

      // If view is updated, descriptor (sampler) has to be recreated
      std::string debugResource = exeParams.rc->getDebugOptions().debugViewResource;
      uint32_t samplerId = translateNameToBinding(debugResource);
      float mip = (float)exeParams.rc->getDebugOptions().debugMip;

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

      Push push{};
      push.samplerId = samplerId;
      push.mip = mip;

      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        0,
        sizeof(Push),
        &push);

      exeParams.rc->drawMeshId(exeParams.cmdBuffer, _meshId, 6, 1);

      vkCmdEndRendering(*exeParams.cmdBuffer);
    });
}

}