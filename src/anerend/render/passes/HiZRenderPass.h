#pragma once

#include "RenderPass.h"

namespace render {

class HiZRenderPass : public RenderPass
{
public:
  HiZRenderPass();
  ~HiZRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}