#pragma once

#include "RenderPass.h"
#include "../AllocatedBuffer.h"
#include "../Model.h"

namespace render {

  class SSAOBlurRenderPass : public RenderPass
  {
  public:
    SSAOBlurRenderPass();
    ~SSAOBlurRenderPass();

    bool init(RenderContext* renderContext, RenderResourceVault*) override final;

    // Register how the render pass will actually render
    void registerToGraph(FrameGraphBuilder&) override final;

    void cleanup(RenderContext* renderContext, RenderResourceVault* vault) override final;

  private:
    std::vector<VkDescriptorSet> _descriptorSets;

    VkSampler _sampler;

    Model _screenQuad;
    MeshId _meshId;
    RenderableId _renderableId;
  };

}