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

private:
  struct Cascade
  {
    const int surfelPixSize;
    const int numRaysPerSurfel;
    const int sqrtNumRaysPerSurfel;
  };

  std::vector<Cascade> _cascades;

  void registerCascade(FrameGraphBuilder& fgb, RenderContext* rc, int cascade);
};

}