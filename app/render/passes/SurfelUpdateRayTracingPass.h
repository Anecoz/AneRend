#pragma once

#include "RenderPass.h"

namespace render {

class SurfelUpdateRayTracingPass : public RenderPass
{
public:
  SurfelUpdateRayTracingPass();
  ~SurfelUpdateRayTracingPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}