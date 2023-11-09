#pragma once

#include "RenderPass.h"

namespace render {

class DebugBSRenderPass : public RenderPass
{
public:
  DebugBSRenderPass();
  ~DebugBSRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}