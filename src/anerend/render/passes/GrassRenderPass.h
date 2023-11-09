#pragma once

#include "RenderPass.h"

namespace render {

class GrassRenderPass : public RenderPass
{
public:
  GrassRenderPass();
  ~GrassRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}