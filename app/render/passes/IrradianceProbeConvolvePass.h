#pragma once

#include "RenderPass.h"

namespace render {

class IrradianceProbeConvolvePass : public RenderPass
{
public:
  IrradianceProbeConvolvePass();
  ~IrradianceProbeConvolvePass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;

private:
  const double _traceRate = 30.0; // in hertz
  double _lastRayTraceTime = 0;
};

}