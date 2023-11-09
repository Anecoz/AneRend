#pragma once

#include "RenderPass.h"

namespace render {

class GeometryRenderPass : public RenderPass
{
public:
  GeometryRenderPass();
  ~GeometryRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}