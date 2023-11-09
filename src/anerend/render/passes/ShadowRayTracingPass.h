#pragma once

#include "RenderPass.h"

namespace render {

class ShadowRayTracingPass : public RenderPass
{
public:
  ShadowRayTracingPass();
  ~ShadowRayTracingPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}