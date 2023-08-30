#pragma once

#include "RenderPass.h"

namespace render {

class ParticleUpdatePass : public RenderPass
{
public:
  ParticleUpdatePass();
  ~ParticleUpdatePass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}