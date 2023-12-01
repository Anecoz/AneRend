#pragma once

#include "RenderPass.h"

namespace render {

class CompactDrawsPass : public RenderPass
{
public:
  CompactDrawsPass();
  ~CompactDrawsPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}