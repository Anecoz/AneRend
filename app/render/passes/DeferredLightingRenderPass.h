#pragma once

#include "RenderPass.h"
#include "../Model.h"

namespace render {

class DeferredLightingRenderPass : public RenderPass
{
public:
  DeferredLightingRenderPass();
  ~DeferredLightingRenderPass();

  bool init(RenderContext* renderContext, RenderResourceVault*) override final;

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&) override final;

  void cleanup(RenderContext* renderContext, RenderResourceVault* vault) override final;

private:
  std::vector<VkDescriptorSet> _descriptorSets;

  VkSampler _sampler0;
  VkSampler _sampler1;
  //VkSampler _sampler2;
  VkSampler _depthSampler;
  VkSampler _shadowMapSampler;
  VkSampler _ssaoSampler;

  Model _screenQuad;
  MeshId _meshId;
  RenderableId _renderableId;
};

}