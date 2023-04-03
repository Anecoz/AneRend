#pragma once

#include "RenderPass.h"

namespace render {

class PresentationRenderPass : public RenderPass
{
public:
  PresentationRenderPass();
  ~PresentationRenderPass();

  bool init(RenderContext* renderContext, RenderResourceVault*) override final;

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&) override final;
};

}