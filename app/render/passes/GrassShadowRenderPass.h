#pragma once

#include "RenderPass.h"

namespace render {

class GrassShadowRenderPass : public RenderPass
{
public:
  GrassShadowRenderPass();
  ~GrassShadowRenderPass();

  bool init(RenderContext* renderContext, RenderResourceVault*) override final;

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&) override final;

  void cleanup(RenderContext* renderContext, RenderResourceVault* vault) override final;

private:
  std::vector<VkDescriptorSet> _descriptorSets;
};

}