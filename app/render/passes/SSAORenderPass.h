#pragma once

#include "RenderPass.h"
#include "../AllocatedBuffer.h"
#include "../Model.h"

namespace render {

class SSAORenderPass : public RenderPass
{
public:
  SSAORenderPass();
  ~SSAORenderPass();

  bool init(RenderContext* renderContext, RenderResourceVault*) override final;

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&) override final;

  void cleanup(RenderContext* renderContext, RenderResourceVault* vault) override final;

private:
  std::vector<VkDescriptorSet> _descriptorSets;

  VkSampler _sampler0;
  VkSampler _depthSampler;
  VkSampler _noiseSampler;
  AllocatedBuffer _sampleBuffer;

  Model _screenQuad;
  MeshId _meshId;
  RenderableId _renderableId;
};

}