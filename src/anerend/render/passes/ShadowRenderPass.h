#pragma once

#include "RenderPass.h"

namespace render {

class ShadowRenderPass : public RenderPass
{
public:
  ShadowRenderPass();
  ~ShadowRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;

private:
  void registerDirectionalShadowPass(FrameGraphBuilder&, RenderContext* rc);
  void registerTerrainDirectionalShadowPass(FrameGraphBuilder&, RenderContext* rc);
  void registerPointShadowPass(FrameGraphBuilder&, RenderContext* rc);
};

}