#pragma once

#include "RenderPass.h"
#include "AllocatedBuffer.h"

namespace render {

class CullRenderPass : public RenderPass
{
public:
  CullRenderPass();
  ~CullRenderPass();

  bool init(RenderContext* renderContext, RenderResourceVault*) override final;

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&) override final;

  void cleanup(RenderContext* renderContext, RenderResourceVault* vault) override final;

private:
  std::vector<AllocatedBuffer> _gpuStagingBuffers;
  std::vector<VkDescriptorSet> _descriptorSets;
};

}