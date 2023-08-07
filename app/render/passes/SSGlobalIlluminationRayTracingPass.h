#pragma once

#include "RenderPass.h"

namespace render {

class SSGlobalIlluminationRayTracingPass : public RenderPass
{
public:
  SSGlobalIlluminationRayTracingPass();
  ~SSGlobalIlluminationRayTracingPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}