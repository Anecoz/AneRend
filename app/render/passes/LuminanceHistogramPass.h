#pragma once

#include "RenderPass.h"

namespace render {

class LuminanceHistogramPass : public RenderPass
{
public:
  LuminanceHistogramPass();
  ~LuminanceHistogramPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}