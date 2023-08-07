#pragma once

#include "RenderPass.h"

namespace render {

class SpecularGIMipPass : public RenderPass
{
public:
  SpecularGIMipPass();
  ~SpecularGIMipPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}