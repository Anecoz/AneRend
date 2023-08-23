#pragma once

#include "RenderPass.h"

namespace render {

class LightShadowRayTracingPass : public RenderPass
{
public:
  LightShadowRayTracingPass();
  ~LightShadowRayTracingPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}