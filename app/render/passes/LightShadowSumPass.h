#pragma once

#include "RenderPass.h"

namespace render {

class LightShadowSumPass : public RenderPass
{
public:
  LightShadowSumPass();
  ~LightShadowSumPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}