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
};

}