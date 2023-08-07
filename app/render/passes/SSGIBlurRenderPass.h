#pragma once

#include "RenderPass.h"

namespace render {

class SSGIBlurRenderPass : public RenderPass
{
public:
  SSGIBlurRenderPass();
  ~SSGIBlurRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}