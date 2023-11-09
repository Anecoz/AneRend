#pragma once

#include "RenderPass.h"

namespace render {

class GrassShadowRenderPass : public RenderPass
{
public:
  GrassShadowRenderPass();
  ~GrassShadowRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}