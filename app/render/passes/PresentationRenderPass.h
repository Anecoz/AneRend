#pragma once

#include "RenderPass.h"

namespace render {

class PresentationRenderPass : public RenderPass
{
public:
  PresentationRenderPass();
  ~PresentationRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}