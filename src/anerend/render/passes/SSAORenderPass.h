#pragma once

#include "RenderPass.h"

namespace render {

class SSAORenderPass : public RenderPass
{
public:
  SSAORenderPass();
  ~SSAORenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}