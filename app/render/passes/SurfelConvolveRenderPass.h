#pragma once

#include "RenderPass.h"

namespace render {

class SurfelConvolveRenderPass : public RenderPass
{
public:
  SurfelConvolveRenderPass();
  ~SurfelConvolveRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}