#pragma once

#include "RenderPass.h"

namespace render {

class SpecularGIRTPass : public RenderPass
{
public:
  SpecularGIRTPass();
  ~SpecularGIRTPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;

private:
  const double _traceRate = 600.0; // in hertz
  double _lastRayTraceTime = 0;
};

}