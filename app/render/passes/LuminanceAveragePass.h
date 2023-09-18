#pragma once

#include "RenderPass.h"

namespace render {

class LuminanceAveragePass : public RenderPass
{
public:
  LuminanceAveragePass();
  ~LuminanceAveragePass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}