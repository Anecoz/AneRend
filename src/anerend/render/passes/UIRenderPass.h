#pragma once

#include "RenderPass.h"

namespace render {

class UIRenderPass : public RenderPass
{
public:
  UIRenderPass();
  ~UIRenderPass();

  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}