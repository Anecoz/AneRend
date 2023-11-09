#pragma once

#include "RenderPass.h"

namespace render {

class BloomRenderPass : public RenderPass
{
public:
  BloomRenderPass();
  ~BloomRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;

private:
  void prefilterPass(FrameGraphBuilder& fgb, RenderContext* rc);
  void downsamplePass(FrameGraphBuilder& fgb, RenderContext* rc);
  void upsamplePass(FrameGraphBuilder& fgb, RenderContext* rc);
  void compositePass(FrameGraphBuilder& fgb, RenderContext* rc);
};

}