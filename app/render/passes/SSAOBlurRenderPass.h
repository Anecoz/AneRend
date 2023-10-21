#pragma once

#include "RenderPass.h"
#include "../AllocatedBuffer.h"
#include "../asset/Mesh.h"

namespace render {

  class SSAOBlurRenderPass : public RenderPass
  {
  public:
    SSAOBlurRenderPass();
    ~SSAOBlurRenderPass();

    // Register how the render pass will actually render
    void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
  };

}