#pragma once

#include "RenderPass.h"

namespace render {

class SurfelSHPass : public RenderPass
{
public:
  SurfelSHPass();
  ~SurfelSHPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;

};

}